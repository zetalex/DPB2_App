import zmq
import time
import sys
import json
import os
import subprocess
import threading
import signal
import sys
import os
from time import sleep
import queue

try:
    import psutil
    PSUTIL_AVAILABLE = True
except ImportError:
    PSUTIL_AVAILABLE = False
    print("Warning: psutil not available. CPU affinity will not be set.")

class DPBHostSoftware:
    # Global variables
    threads = []
    running = threading.Event()
    running.clear()  # Set to False initially
    destroy = threading.Event()
    destroy.clear()  # Set to False initially

    # TCP Buffer sizes (in bytes)
    TCP_RECV_BUFFER_SIZE = 2 * 1024 * 1024  # 2MB receive buffer
    TCP_SEND_BUFFER_SIZE = 2 * 1024 * 1024  # 2MB send buffer
    ZMQ_RECV_HWM = 10000  # High water mark for receiving
    ZMQ_SEND_HWM = 10000  # High water mark for sending
    
    output_file = None
    mutex_lock_data = threading.Lock()
    
    # The constructor method to initialize new objects
    def __init__(self, dpb_ip, monitoring_port=5555, alarm_port=5556, cmd_port=5557, config_port=5559, logging_port=5558, data_port=5570, log_file="dpb_logging.txt"):
        """
        Initialize DPBHostSoftware instance and establish connections to DPB.
        
        Args:
            dpb_ip (str): IP address of the DPB device
            monitoring_port (int): Port for monitoring data (Default 5555)
            alarm_port (int): Port for alarm triggering (Default 5556)
            cmd_port (int): Port for command interface (Default 5557)
            config_port (int): Port for configuration interface (Default 5559)
            logging_port (int): Port for logging data (Default 5558)
            data_port (int): Port for data acquisition (Default 5570)
            log_file (str): File to store logging data from DPB (Default "dpb_logging.txt")
            
        Return:
            DPB HostSoftware instance
        """
        self.dpb_ip = dpb_ip          # Attribute to store DPB IP address
        self.cmd_port = cmd_port      # Attribute to store command port
        self.alarm_port = alarm_port  # Attribute to store alarm port
        self.config_port = config_port # Attribute to store configuration port
        self.data_port = data_port     # Attribute to store data port
        self.monitoring_port = monitoring_port  # Attribute to store monitoring port
        self.logging_port = logging_port  # Attribute to store logging port
        self.log_file = log_file      # Attribute to store log file name
        
        # Initialize ZMQ context and sockets
        self.context = zmq.Context()
        self.socket_cmd = self.context.socket(zmq.REQ)
        self.socket_config = self.context.socket(zmq.REQ)
        self.socket_monitoring = self.context.socket(zmq.SUB)
        self.socket_logging = self.context.socket(zmq.SUB)
        self.socket_cmd.setsockopt(zmq.RCVTIMEO, -1)
        self.socket_alarm = self.context.socket(zmq.SUB)
        self.socket_alarm.setsockopt(zmq.RCVTIMEO, -1)
        self.socket_config.setsockopt(zmq.RCVTIMEO, -1)
        self.socket_monitoring.setsockopt(zmq.RCVTIMEO, -1)
        self.socket_monitoring.setsockopt_string(zmq.SUBSCRIBE, "")
        self.socket_logging.setsockopt_string(zmq.SUBSCRIBE, "")
        self.socket_alarm.setsockopt_string(zmq.SUBSCRIBE, "")

        # Check DPB connectivity
        ping_result = os.system(f"ping -c 5 {self.dpb_ip} > /dev/null 2>&1")
        if(ping_result != 0):
            raise ConnectionError(f"Cannot reach DPB at IP {self.dpb_ip}")
        dpb_ver = self.send_ssh_command("cat /etc/dpb_os_ver")
        if "Xilinx Petalinux" not in dpb_ver:
            raise EnvironmentError(f"IP {self.dpb_ip} is not a DPB device")
        print(f"DPB found at {self.dpb_ip}: \n{dpb_ver}")
        
        # Start Slow control software and data taking application on DPB
        self.send_ssh_command("systemctl start dpb-slowcontrolapp")
        self.socket_cmd.connect(f"tcp://{self.dpb_ip}:{self.cmd_port}")
        self.socket_config.connect(f"tcp://{self.dpb_ip}:{self.config_port}")
        self.socket_monitoring.connect(f"tcp://{self.dpb_ip}:{self.monitoring_port}")
        self.socket_logging.connect(f"tcp://{self.dpb_ip}:{self.logging_port}")
        self.socket_alarm.connect(f"tcp://{self.dpb_ip}:{self.alarm_port}")
        # Start logging thread
        self.log_thread = threading.Thread(
            target=self.__logging_thread,
            daemon=True
        )

        # Start alarm thread
        self.alarm_thread = threading.Thread(
            target=self.__alarm_thread,
            daemon=True
        )
        self.log_thread.start()
        self.alarm_thread.start()
        self.send_ssh_command("systemctl start daq-readout")
        time.sleep(10)  # Wait for the service to start
        
        # Keep sending messages until connection to slow control app is established
        retries = 0
        while retries < 20:
            response = self.send_slow_control_command("READ DPB TEMP PCB")
            if response != -1:
                break
            retries += 1
            time.sleep(1)
        if retries == 20:
            raise ConnectionError(f"Cannot connect to slow control app on DPB at IP {self.dpb_ip}")
        # Start Data Taking ZMQ sockets
        self.__start_data_threads()
        print(f"DPB at {self.dpb_ip} is ready")

    def __del__(self):
        self.__stop_data_threads()
        self.socket_cmd.close()
        self.socket_config.close()
        self.socket_monitoring.close()
        self.context.term()

    def __logging_thread(self):
        """Thread to handle logging from DPB"""
        try:
            with open(self.log_file, 'a') as f:
                while not self.destroy.is_set():
                    try:
                        log_data = self.socket_logging.recv_string(flags=zmq.NOBLOCK)
                        timestamp = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())
                        f.write(f"[{timestamp}] {log_data}\n")
                        f.flush()
                    except zmq.Again:
                        time.sleep(0.1)  # No message, wait a bit
        except Exception as e:
            print(f"Logging thread error: {e}")
            
    def __alarm_thread(self):
        """Thread to handle alarms from DPB"""
        try:
            with open(self.log_file, 'a') as f:
                while not self.destroy.is_set():
                    try:
                        timestamp = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())
                        alarm_data = self.socket_alarm.recv_string(flags=zmq.NOBLOCK)
                        print(f"Alarm received at {timestamp}!")
                        f.write(f"[{timestamp}] {alarm_data}\n")
                        f.flush()
                    except zmq.Again:
                        time.sleep(0.1)  # No message, wait a bit
        except Exception as e:
            print(f"Alarm thread error: {e}")

    def read_config_json_file(file_path):
        """
        Read and validate JSON content from the specified file.
        
        Args:
            filename (str): Name of the JSON file to read
            
        Returns:
            str: JSON content as string, or None if error occurred
        """
        try:
            with open(file_path, 'r', encoding='utf-8') as file:
                content = file.read()
                
            # Validate that it's proper JSON
            json.loads(content)
            return content
            
        except FileNotFoundError:
            print(f"Error: File '{file_path}' not found.")
            return None
        except json.JSONDecodeError as e:
            print(f"Error: Invalid JSON in file '{file_path}': {e}")
            return None
        except Exception as e:
            print(f"Error reading file '{file_path}': {e}")
            return None
    
    def send_ssh_command(self, command):
        print(f"Executing '{command}' on {self.dpb_ip} via SSH.")
        return subprocess.getoutput(f"sshpass -p root ssh root@{self.dpb_ip} '{command}'")

    def send_slow_control_command(self, command):
        msg = "{'msg_id':0, 'msg_time':'2021-11-19T17:54:30.691Z', 'msg_type':'Command', 'msg_value':'" + command + "', 'uuid': '931fbc9d-b2b3-c248-87d6ae33f9a62'}"
        try: 
            self.socket_cmd.send_string(msg)
            response = self.socket_cmd.recv_string()
            json_obj = json.loads(response)
            value = json_obj["msg_value"]
        except zmq.Again:
            raise TimeoutError("Timeout waiting for response from DPB command")
        return value

    def send_configuration_json(self, config_json):
        try:
            print("Reading JSON content...")
            json_content = self.read_config_json_file(config_json)
            if not json_content:
                print("Failed to read JSON content. Exiting.")
                sys.exit(1)
            
            print("JSON content loaded successfully.")
            print(f"Content preview: {json_content[:100]}..." if len(json_content) > 100 else f"Content: {json_content}")
            self.socket_config.send_string(json_content)
            response = self.socket_config.recv_string()
        except zmq.Again:
            raise TimeoutError("Timeout waiting for response from DPB config")
        finally:
            return response

    def get_mon_data(self):
        try:
            json_data = self.socket_monitoring.recv_string()
            response = json.dumps(json.loads(json_data), indent=4)
            return response
        except zmq.Again:
            return -1
    
    def get_dig_data(self, time_ms):
        # Get semaphore to use this function only once at a time
        mutex_acquired = self.mutex_lock_data.acquire(blocking=False)
        if not mutex_acquired:
            print("Data acquisition is already in progress.")
            return None
        
        # Start data acquisition on DPB
        self.send_slow_control_command("SET DPB STATUS DMA ON")
        
        # Get digital data from the DPB
        self.running.set()
        time.sleep(time_ms / 1000.0)
        self.running.clear()
        
        # Stop data acquisition on DPB
        self.send_slow_control_command("SET DPB STATUS DMA OFF")
        sleep(1)  # Wait to ensure all data is written
        
        # Read data from file
        with open("temp.bin", 'rb') as f:
            data = f.read()
        os.remove("temp.bin")
        self.mutex_lock_data.release()
        return data

    def __worker_thread(self, thread_id, cpu_core, data_queue, server_ip, server_port):
        """Function that executes each worker thread"""
        # Set CPU affinity for this thread
        if PSUTIL_AVAILABLE:
            # Get current process
            process = psutil.Process()
            # Set CPU affinity to specific core
            process.cpu_affinity([cpu_core])
            # print(f"Thread {thread_id}: Assigned to CPU core {cpu_core}")
        else:
            # Alternative method using os.sched_setaffinity (Linux only)
            import os
            # Set CPU affinity using os.sched_setaffinity
            os.sched_setaffinity(0, {cpu_core})
            # print(f"Thread {thread_id}: Assigned to CPU core {cpu_core} (using os.sched_setaffinity)")
        
        # Each thread can share the same ZMQ context in Linux
        worker_socket = None
        
        try:
            worker_socket = self.context.socket(zmq.ROUTER)
            
            # Configure TCP buffer sizes for better performance
            worker_socket.setsockopt(zmq.RCVBUF, self.TCP_RECV_BUFFER_SIZE)
            worker_socket.setsockopt(zmq.SNDBUF, self.TCP_SEND_BUFFER_SIZE)
            worker_socket.setsockopt(zmq.RCVHWM, self.ZMQ_RECV_HWM)
            worker_socket.setsockopt(zmq.SNDHWM, self.ZMQ_SEND_HWM)
            worker_socket.setsockopt(zmq.TCP_KEEPALIVE, 1)
            worker_socket.setsockopt(zmq.TCP_KEEPALIVE_IDLE, 600)  # 10 minutes
            worker_socket.setsockopt(zmq.TCP_KEEPALIVE_INTVL, 60)  # 1 minute
            worker_socket.setsockopt(zmq.TCP_KEEPALIVE_CNT, 3)
            worker_socket.setsockopt(zmq.RCVTIMEO, -1)  # Infinite timeout
            
            url = f"tcp://{server_ip}:{server_port}"
            worker_socket.connect(url)
            while not self.destroy.is_set():
                while self.running.is_set():
                    try:
                        message = worker_socket.recv_multipart()
                        
                        # Only write the payload, not all frames
                        if len(message) > 0:
                            # Usually the last frame contains the actual data
                            payload = message[-1]  # Take only the last frame
                            data_queue.put(payload)
                            
                    except zmq.Again:
                        continue
                    except zmq.ContextTerminated:
                        break
                    except Exception as e:
                        if self.running.is_set():  # Only show error if we're not closing
                            print(f"Thread {thread_id}: Error - {e}")
                        break
        except Exception as e:
            print(f"Thread {thread_id}: Connection error - {e}")
        finally:
            try:
                if worker_socket:
                    worker_socket.close()
            except:
                pass
            print(f"Thread {thread_id}: Terminated correctly")

    def __file_writer_thread(self, data_queue):
        """Dedicated thread for writing to file"""
        try:
            while not self.destroy.is_set():
                while self.running.is_set() or not data_queue.empty():
                    try:
                        data = data_queue.get(timeout=0.1)
                        self.output_file.write(data)
                        self.output_file.flush()
                        data_queue.task_done()
                    except queue.Empty:
                        continue
                    except Exception as e:
                        print(f"File writer error: {e}")
                        break
        finally:
            self.output_file.close()

    def __start_data_threads(self):
        
        # Parse command line arguments
        # parser = argparse.ArgumentParser(description='High-performance ZMQ event receiver')
        # parser.add_argument('--dpb-ip', required=True, help='DPB server IP address', dest='server_ip')
        # parser.add_argument('--port', required=True, help='DPB server port', dest='server_port')
        # parser.add_argument('--out', required=True, help='Output file to write received data', dest='output_file')
        # args = parser.parse_args()
        
        # Open output file for writing
        try:
            self.output_file = open("temp.bin", 'wb')  # Open in binary mode
        except Exception as e:
            print(f"Error opening output file: {e}")
            sys.exit(1)
        
        # Thread-safe queue for data
        data_queue = queue.Queue(maxsize=10000)
        
        # Get available CPU cores
        num_cores = os.cpu_count()
        if num_cores is None:
            num_cores = 8  # fallback

        # Create thread for file writing
        writer_thread = threading.Thread(
            target=self.__file_writer_thread, 
            args=[data_queue], 
            daemon=True
        )
        writer_thread.start()
        self.threads.append(writer_thread)
        
        ##
        # Create and launch 8 worker threads
        
        for i in range(8):
            # Assign each thread to a different CPU core (round-robin)
            cpu_core = i % num_cores
            thread = threading.Thread(
                target=self.__worker_thread, 
                args=(i+1, cpu_core, data_queue, self.dpb_ip, self.data_port), 
                daemon=True
            )
            thread.start()
            self.threads.append(thread)

    def __stop_data_threads(self):
        self.running.clear()
        self.destroy.set()   
        # Wait for threads to finish
        for i, thread in enumerate(self.threads):
            if thread.is_alive():
                thread.join(timeout=3.0)
                if thread.is_alive():
                    print(f"Thread {i+1} didn't finish gracefully")
            
        # Close output file
        try:
            self.output_file.close()
        except:
            pass
