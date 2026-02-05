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

    # TCP Buffer sizes (in bytes) - Optimized for high throughput
    TCP_RECV_BUFFER_SIZE = 16 * 1024 * 1024  # 16MB receive buffer
    TCP_SEND_BUFFER_SIZE = 16 * 1024 * 1024  # 16MB send buffer
    ZMQ_RECV_HWM = 100000  # High water mark for receiving (increased 10x)
    ZMQ_SEND_HWM = 100000  # High water mark for sending (increased 10x)
    BATCH_SIZE = 100  # Number of messages to batch before writing
    
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
        """
        Destructor to cleanup resources and close connections.
        
        @return None
        """
        self.__stop_data_threads()
        self.socket_cmd.close()
        self.socket_config.close()
        self.socket_monitoring.close()
        self.context.term()

    def __logging_thread(self):
        """
        Thread to handle logging messages from DPB.
        
        Continuously receives logging messages from the DPB via ZMQ socket and writes them
        to the log file with timestamps. Runs until destroy event is set.
        
        @return None
        """
        try:
            with open(self.log_file, 'a') as f:
                while not self.destroy.is_set():
                    try:
                        log_data = self.socket_logging.recv_string(flags=zmq.NOBLOCK)
                        timestamp = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())
                        f.write(f"[{timestamp}] {log_data}")
                        f.flush()
                    except zmq.Again:
                        time.sleep(0.1)  # No message, wait a bit
        except Exception as e:
            print(f"Logging thread error: {e}")
            
    def __alarm_thread(self):
        """
        Thread to handle alarm messages from DPB.
        
        Continuously receives alarm messages from the DPB via ZMQ socket and writes them
        to the log file with timestamps. Runs until destroy event is set.
        
        @return None
        """
        try:
            with open(self.log_file, 'a') as f:
                while not self.destroy.is_set():
                    try:
                        timestamp = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())
                        alarm_data = self.socket_alarm.recv_string(flags=zmq.NOBLOCK)
                        print(f"Alarm received at {timestamp}: {time.time()}")
                        f.write(f"[{timestamp}] {alarm_data}\n")
                        f.flush()
                    except zmq.Again:
                        time.sleep(0.1)  # No message, wait a bit
        except Exception as e:
            print(f"Alarm thread error: {e}")

    def read_config_json_file(self, file_path):
        """
        Read and validate JSON content from the specified file.
        
        Opens and reads a JSON file, validates its format and returns the content as a string.
        
        @param file_path Path to the JSON configuration file to read
        @return JSON content as string, or None if error occurred
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
        """
        Execute a command on the DPB via SSH.
        
        Connects to the DPB using SSH with sshpass and executes the specified command.
        Uses root credentials for authentication.
        
        @param command Shell command string to execute on the DPB
        @return Command output as string
        """
        print(f"Executing '{command}' on {self.dpb_ip} via SSH.")
        return subprocess.getoutput(f"sshpass -p root ssh root@{self.dpb_ip} '{command}'")

    def send_slow_control_command(self, command):
        """
        Send a slow control command to the DPB via ZMQ socket.
        
        Wraps the command in a JSON message structure and sends it via ZMQ REQ socket.
        Waits for and returns the response value.
        
        @param command Slow control command string to send to the DPB
        @return Response value from the DPB, or raises TimeoutError if no response received
        @throws TimeoutError If timeout occurs waiting for DPB response
        """
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
        """
        Send a JSON configuration file to the DPB.
        
        Reads the specified JSON configuration file, validates it and sends it to the DPB
        via the configuration ZMQ socket. Waits for confirmation response.
        
        @param config_json Path to the JSON configuration file to send
        @return Response string from the DPB
        @throws TimeoutError If timeout occurs waiting for DPB response
        """
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
        """
        Retrieve monitoring data from the DPB.
        
        Receives the latest monitoring data JSON from the DPB via the monitoring ZMQ socket
        and returns it formatted with indentation.
        
        @return Formatted JSON string with monitoring data, or -1 if no data available
        """
        try:
            json_data = self.socket_monitoring.recv_string()
            response = json.dumps(json.loads(json_data), indent=4)
            return response
        except zmq.Again:
            return -1
    
    def get_dig_data(self, time_ms):
        """
        Acquire digitizer data from the DPB for a specified duration.
        
        Enables DMA on the DPB, collects data for the specified time period, then stops DMA
        and retrieves the collected binary data. Uses mutex to ensure only one acquisition
        runs at a time.
        
        @param time_ms Duration in milliseconds to collect data
        @return Binary data as bytes object, or None if acquisition already in progress
        """
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
        """
        Worker thread function for receiving data from DPB.
        
        Each worker thread connects to the DPB data port via ZMQ ROUTER socket, sets CPU
        affinity to a specific core for performance, and continuously receives data messages
        while the running flag is set. Received data is placed in the shared queue for writing.
        
        @param thread_id Unique identifier for this worker thread
        @param cpu_core CPU core number to bind this thread to for affinity
        @param data_queue Thread-safe queue to store received data for file writing
        @param server_ip IP address of the DPB server
        @param server_port Port number for data connection
        @return None
        """
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
            
            # TCP Keepalive settings
            worker_socket.setsockopt(zmq.TCP_KEEPALIVE, 1)
            worker_socket.setsockopt(zmq.TCP_KEEPALIVE_IDLE, 600)
            worker_socket.setsockopt(zmq.TCP_KEEPALIVE_INTVL, 60)
            worker_socket.setsockopt(zmq.TCP_KEEPALIVE_CNT, 3)
            worker_socket.setsockopt(zmq.RCVTIMEO, -1)
            
            # Set linger to 0 for faster cleanup
            worker_socket.setsockopt(zmq.LINGER, 0)
            
            url = f"tcp://{server_ip}:{server_port}"
            worker_socket.connect(url)
            
            batch = []  # Batch messages for better queue performance
            
            while not self.destroy.is_set():
                while self.running.is_set():
                    try:
                        # Use copy=False to avoid unnecessary memory copies (zero-copy)
                        # recv_multipart with copy=False is faster for large messages
                        message = worker_socket.recv_multipart(copy=False)
                        
                        # Extract payload from frames
                        if len(message) > 0:
                            # Last frame contains actual data - convert Frame to bytes
                            payload = bytes(message[-1])
                            batch.append(payload)
                            
                            # Put batch into queue when it reaches BATCH_SIZE
                            if len(batch) >= self.BATCH_SIZE:
                                data_queue.put(batch)
                                batch = []
                            
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
            # Flush any remaining batched messages
            if batch:
                data_queue.put(batch)
            try:
                if worker_socket:
                    worker_socket.close()
            except:
                pass
            print(f"Thread {thread_id}: Terminated correctly")

    def __file_writer_thread(self, data_queue):
        """
        Dedicated thread for writing received data to file.
        
        Continuously retrieves batches of data from the queue and writes them to file.
        Uses buffered writes and periodic flushing for better performance.
        
        @param data_queue Thread-safe queue containing lists of binary data to write
        @return None
        """
        try:
            write_count = 0
            flush_interval = 1000  # Flush every N batches for better performance
            
            while not self.destroy.is_set():
                while self.running.is_set() or not data_queue.empty():
                    try:
                        # Get batch of messages
                        batch = data_queue.get(timeout=0.1)
                        
                        # Write all messages in batch
                        for data in batch:
                            self.output_file.write(data)
                        
                        write_count += 1
                        
                        # Periodic flush instead of flushing after every write
                        if write_count >= flush_interval:
                            self.output_file.flush()
                            write_count = 0
                        
                        data_queue.task_done()
                    except queue.Empty:
                        # Flush on timeout to ensure data is written
                        if write_count > 0:
                            self.output_file.flush()
                            write_count = 0
                        continue
                    except Exception as e:
                        print(f"File writer error: {e}")
                        break
        finally:
            # Final flush before closing
            self.output_file.flush()
            self.output_file.close()

    def __start_data_threads(self):
        """
        Initialize and start data acquisition thread.
        
        Creates a temporary binary output file, initializes a thread-safe queue for data,
        and launches one receiver thread plus one file writer thread.
        Uses a single ZMQ socket to maintain FIFO message ordering automatically.
        
        @return None
        """

        # Open output file for writing
        try:
            self.output_file = open("temp.bin", 'wb')  # Open in binary mode
        except Exception as e:
            print(f"Error opening output file: {e}")
            sys.exit(1)
        
        # Thread-safe queue for data
        data_queue = queue.Queue(maxsize=10000)

        # Create single receiver thread
        receiver_thread = threading.Thread(
            target=self.__worker_thread, 
            args=(1, 0, data_queue, self.dpb_ip, self.data_port), 
            daemon=True
        )
        receiver_thread.start()
        self.threads.append(receiver_thread)
        
        # Create thread for file writing
        writer_thread = threading.Thread(
            target=self.__file_writer_thread, 
            args=[data_queue], 
            daemon=True
        )
        writer_thread.start()
        self.threads.append(writer_thread)

    def __stop_data_threads(self):
        """
        Stop all data acquisition threads and cleanup resources.
        
        Clears the running flag, sets the destroy event, and waits for all threads to
        terminate gracefully with a timeout. Closes the output file after all threads finish.
        
        @return None
        """
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
