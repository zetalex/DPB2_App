from ctypes import *
from ctypesdata import *
import ctypes
import re
import os
import zmq
import sys
import json
import subprocess
from robot.api import logger
from robotremoteserver import RobotRemoteServer


class DPB2scLibrary(object):
    def find_and_load_library(self,pattern, directory):
        """Searches library in determiend directory and loads it for ctypes.

        Args:
        pattern (str): Name of the library.

        Args:
        directory (str): Directory where the library is to be found.
        
        """ 
        files = os.listdir(directory)
        for file_name in files:
            if re.match(pattern, file_name):
                file_path = os.path.join(directory, file_name)
                try:
                    return ctypes.CDLL(file_path, mode=RTLD_GLOBAL)
                except OSError:
                    continue
        return None

    library_directory = "/usr/lib"
    """ Libraries directory
    """ 
    function_defs = {}
    """ Array of C functions in ctypes
    """ 
    structure_i2c = DPB_I2cSensors()
    """ I2C Devices Structure
    """ 
    #########################################################
    # Initialization functions
    #########################################################
    def __init__(self):
        """Initializes Remote Server, library and necessary resources of the DPB.
        
        """ 
        self.find_and_load_library(r'^libjson-c\.so', self.library_directory)
        self.find_and_load_library(r'^libzmq\.so', self.library_directory)
        self.dpb2sc = ctypes.CDLL("libdpb2sc.so")
        start_line = 0
        end_line = 0

        with open(r'/usr/include/dpb2sc.h', 'r') as fp:
            # read all lines using readline()func_name
            lines = fp.readlines()
            for row in lines:
                # check if string present on a current line
                word1 = 'Function Prototypes'
                word2 = 'Constant Definitions'
                #print(row.find(word))
                # find() method returns -1 if the value is not found,
                # if found it returns index of the first occurrence of the substring
                if row.find(word1) != -1:  
                    start_line = lines.index(row)+2
                if row.find(word2) != -1:
                    end_line = lines.index(row)-3
        fp.close()
        with open(r'/usr/include/dpb2sc.h', 'r') as fp:
            for _ in range(start_line):
                next(fp)
            for line_num, line in enumerate(fp, start_line):
                if start_line <= line_num <= end_line:
                    match = re.match(r'\s*(\w+)\s+(\w+)\s*\((.*?)\);', line.strip())
                    if match:
                        return_type, func_name, args = match.groups()
                        args = [arg.strip() for arg in args.split(',')]
                        argtypes = []
                        for arg in args:
                            # Si es un puntero a char
                            if arg.endswith('*') and arg.strip('*') in ['char', 'const char','char ', 'const char ']:
                                argtypes.append(ctypes.c_char_p)
                            # Si es un puntero doble a char
                            elif arg.endswith('*') and arg.strip('*') == 'char*':
                                argtypes.append(ctypes.POINTER(ctypes.c_char_p))
                            # Si es otro tipo de puntero
                            elif '*' in arg:
                                arg = arg.replace('*', '')
                                argtypes.append(ctypes.POINTER(ctype_map[arg]))
                            # Si no es un puntero
                            elif arg == '':
                                argtypes.append(None)
                            else:
                                argtypes.append(ctype_map[arg])
                        self.function_defs[func_name] = {
                            'argtypes': argtypes,
                            'restype': ctype_map[return_type]
                        }
        fp.close()

    def __del__(self):
        """Destroys class
        """ 
        self.dpb2sc.sighandler(ctypes.c_int(15))

    def initialize_zmq_ethernet_sockets (self):
        """Initializes DPB ZMQ sockets.
        """ 
        self._zmq_rc = self.dpb2sc.zmq_socket_init()
    def initialize_i2c_devices (self):
        """Initializes DPB I2C Devices.
        """ 
        self.context = zmq.Context()
        self.socket = self.context.socket(zmq.SUB)
        self.socket.connect("tcp://127.0.0.1:5556")
        self.socket.setsockopt_string(zmq.SUBSCRIBE, "")  
        self.dpb2sc.init_I2cSensors(byref(self.structure_i2c))
    def initialize_iio_event_monitor (self):
        """Initializes DPB IIO Event Monitor.
        """ 
        self.dpb2sc.iio_event_monitor_up()
    def get_gpio_base_address (self):
        """Gets DPB GPIO Base Address.
        """ 
        self.GPIO_Base_Address = c_int.in_dll(self.dpb2sc, "GPIO_BASE_ADDRESS")
        self.dpb2sc.get_GPIO_base_address(byref(self.GPIO_Base_Address))

    #########################################################
    #Ethernet Links functions
    #########################################################
    def ethernet_speed_test(self,IP_address):
        """Performs a speed test using iperf3 over the Ethernet interface.

        Args:
        IP_address (str): The destination IP address for the speed test.
        
        Returns:
        str: A string containing the results of the speed test.
        """
        aux = 0
        str = 'iperf3 -c ' + IP_address + ' --logfile /home/petalinux/log.txt'
        os.system(str)
        with open(r'/home/petalinux/log.txt', 'r') as fp:
            # read all lines using readline()func_name
            lines = fp.readlines()
            for row in lines:
                word1 = 'iperf Done'
                if row.find(word1) != -1:
                    aux = 1 
                    file_str = fp.read() 
        fp.close()
        os.system('rm /home/petalinux/log.txt')
        if aux == 0:
            raise AssertionError("Iperf3 could not be performed")
        elif aux == 1:
            logger.info(file_str)

    def select_active_ethernet_interface(self,eth_interface):
        """Select active Ethernet interface

        Args:
        eth_interface (string): Ethernet interface.

        """
        str = 'echo /sys/devices/virtual/net/daq-bond/bonding/active_slave > '
        if eth_interface == "Main":
            str += "eth0"
        elif eth_interface == "Backup":
            str += "eth1"
        os.system(str)
    

    def set_ethernet_link_status (self,eth_interface,value):
        """Set Ethernet interface status

        Args:
        eth_interface (string): Ethernet interface.

        Args:
        value (string): Status ON/OFF.

        """
        if eth_interface == "Main":
            c_eth_interface = c_char_p("eth0")
        elif eth_interface == "Backup": 
            c_eth_interface = c_char_p("eth1")
        if value == "ON":
            c_value = c_int(1)
        elif value == "OFF": 
            c_value = c_int(0)
        self.dpb2sc.eth_link_status_config(c_eth_interface,c_value)

    #########################################################
    #INA3221 functions
    #########################################################
    def get_bus_voltage (self,chip):
        """Get Bus Voltage

        Args:
        chip (string): Desired voltage chip.

        """
        FloatPointer = ctypes.POINTER(ctypes.c_float)
        float_array = (ctypes.c_float * 3)
        float_ptr = ctypes.cast(float_array, FloatPointer)
        if chip == "SFP0":
            c_chip = c_int(0)
            channel = 0
        elif chip == "SFP1": 
            c_chip = c_int(0)
            channel = 1
        elif chip == "SFP2": 
            c_chip = c_int(0)
            channel = 2
        elif chip == "SFP3": 
            c_chip = c_int(1)
            channel = 0
        elif chip == "SFP4": 
            c_chip = c_int(1)
            channel = 1
        elif chip == "SFP5": 
            c_chip = c_int(1)
            channel = 2
        elif chip == "12V": 
            c_chip = c_int(2)
            channel = 0
        elif chip == "3V3": 
            c_chip = c_int(2)
            channel = 1
        elif chip == "1V8": 
            c_chip = c_int(2)
            channel = 2
        self.dpb2sc.ina3221_get_voltage(byref(self.structure_i2c),c_chip,float_ptr)
        self._result = float_array[channel].value

    def get_bus_current (self,chip):
        """Get Bus Current

        Args:
        chip (string): Desired current chip.

        """
        FloatPointer = ctypes.POINTER(ctypes.c_float)
        float_array = (ctypes.c_float * 3)
        float_ptr = ctypes.cast(float_array, FloatPointer)
        if chip == "SFP0":
            c_chip = c_int(0)
            channel = 0
        elif chip == "SFP1": 
            c_chip = c_int(0)
            channel = 1
        elif chip == "SFP2": 
            c_chip = c_int(0)
            channel = 2
        elif chip == "SFP3": 
            c_chip = c_int(1)
            channel = 0
        elif chip == "SFP4": 
            c_chip = c_int(1)
            channel = 1
        elif chip == "SFP5": 
            c_chip = c_int(1)
            channel = 2
        elif chip == "12V": 
            c_chip = c_int(2)
            channel = 0
        elif chip == "3V3": 
            c_chip = c_int(2)
            channel = 1
        elif chip == "1V8": 
            c_chip = c_int(2)
            channel = 2
        self.dpb2sc.ina3221_get_current(byref(self.structure_i2c),c_chip,float_ptr)
        self._result = float_array[channel].value


    #########################################################
    #SFPs functions
    #########################################################
    def sfp_tx_power (self,chip):
        """Get SFP TX Power

        Args:
        chip (string): Desired SFP TX Power.

        """
        FloatPointer = ctypes.POINTER(ctypes.c_float)
        float_array = (ctypes.c_float * 1)
        float_ptr = ctypes.cast(float_array, FloatPointer)
        if chip == "SFP0":
            c_chip = c_int(0)
        elif chip == "SFP1": 
            c_chip = c_int(1)
        elif chip == "SFP2": 
            c_chip = c_int(2)
        elif chip == "SFP3": 
            c_chip = c_int(3)
        elif chip == "SFP4": 
            c_chip = c_int(4)
        elif chip == "SFP5": 
            c_chip = c_int(5)
        self.dpb2sc.sfp_avago_read_tx_av_optical_pwr(byref(self.structure_i2c),c_chip,float_ptr)
        self._result = float_array[0].value

    def sfp_rx_power (self,chip):
        """Get SFP RX Power

        Args:
        chip (string): Desired SFP RX Power.

        """
        FloatPointer = ctypes.POINTER(ctypes.c_float)
        float_array = (ctypes.c_float * 1)
        float_ptr = ctypes.cast(float_array, FloatPointer)
        if chip == "SFP0":
            c_chip = c_int(0)
        elif chip == "SFP1": 
            c_chip = c_int(1)
        elif chip == "SFP2": 
            c_chip = c_int(2)
        elif chip == "SFP3": 
            c_chip = c_int(3)
        elif chip == "SFP4": 
            c_chip = c_int(4)
        elif chip == "SFP5": 
            c_chip = c_int(5)
        self.dpb2sc.sfp_avago_read_rx_av_optical_pwr(byref(self.structure_i2c),c_chip,float_ptr)
        self._result = float_array[0].value

    #########################################################
    #GPIO functions
    #########################################################

    def read_gpio (self, pin_num):
        """Read GPIO

        Args:
        pin_num (int): Desired GPIO pin to read

        """
        Int_pointer = POINTER(c_int)
        int_array = (ctypes.c_int * 1)
        int_ptr = ctypes.cast(int_array, Int_pointer)

        c_pin_num = c_int(pin_num)
        self.dpb2sc.read_GPIO(c_pin_num,int_ptr)
        self._result = int_array[0].value
    
    def write_gpio (self, pin_num, value):
        """Write GPIO

        Args:
        pin_num (int): Desired GPIO pin to write.

        Args:
        value (int): Value to write.

        """
        if (self._result < 0 or self._result > 11) and (self._result < 48 or self._result > 65):
            raise AssertionError('Pin number not in valid range. Pin number = %s' % (self._result))
        if value == "ON":
            c_value = c_int(1)
        elif value == "OFF": 
            c_value = c_int(0)
        c_pin_num = c_int(pin_num)
        self.dpb2sc.write_GPIO(c_pin_num,c_value)

    #########################################################
    #AMS functions
    #########################################################
        
    def get_ams_voltage (self, channel):
        """Get AMS Voltage

        Args:
        channel (int): Desired channel to get voltage from

        """
        FloatPointer = ctypes.POINTER(ctypes.c_float)
        float_array = (ctypes.c_float * 1)
        float_ptr = ctypes.cast(float_array, FloatPointer)

        IntPointer = ctypes.POINTER(ctypes.c_int)
        int_array = (ctypes.c_int * 1) (channel)
        int_ptr = ctypes.cast(int_array, IntPointer)

        self.dpb2sc.xlnx_ams_read_volt(int_ptr,c_int(1),float_ptr)
        self._result = float_array[0].value

    def set_ams_alarms_limit (self,magnitude,ev_dir,channel,value):
        """Set AMS alarm limit

        Args:
        channel (int): Desired channel to set limit.

        Args:
        magnitude(string): Channel magnitude.

        Args:
        ev_dir(string): Alarm direction.

        Args:
        value(float): Value to set.

        """
        if magnitude == "Temperature":
            c_magnitude = c_char_p("temp")
        elif magnitude == "Voltage": 
            c_magnitude = c_char_p("voltage")

        if ev_dir == "Upper":
            c_ev_dir = c_char_p("rising")
        elif ev_dir == "Lower": 
            c_ev_dir = c_char_p("falling")

        self.dpb2sc.xlnx_ams_set_limits(c_int(channel),c_ev_dir,c_magnitude,c_float(value))

    #########################################################
    #Command functions
    #########################################################
        
    def execute_dpb_command (self,command):
        """Execute command in DPB

        Args:
        command(string): DPB command.

        """
        self.context = zmq.Context()
        self.socket = self.context.socket(zmq.REQ)
        self.socket.connect("tcp://127.0.0.1:5557")
        palabras = command.split()
        self.socket.send_string("Hola")
    
        char_array = (ctypes.c_char_p * len(palabras))()
        
        for i, palabra in enumerate(palabras):
            char_array[i] = ctypes.c_char_p(palabra.encode())
        
        ptr_ptr_char = ctypes.cast(char_array, ctypes.POINTER(ctypes.c_char_p))

        self.dpb2sc.dpb_command_handling(byref(self.structure_i2c),ptr_ptr_char, c_int(33))
        cmd_reply = self.socket.recv_string()
        mensaje_json = json.loads(cmd_reply)
        self.cmd_msg_value = mensaje_json.get('msg_value')
        self.socket.close()
        self.context.term()

    #########################################################
    #Check functions
    #########################################################
    def result_should_be_within_tolerance_range(self, expected, tolerance):
        """Check if value is within range

        Args:
        expected(float): Expected value.
        
        Args:
        tolerance(string): Expected tolerance with '%'.

        """

        tolerance_decimal = float(tolerance.strip('%')) / 100.0

        lower_bound = expected * (1 - tolerance_decimal)
        upper_bound = expected * (1 + tolerance_decimal)

        if not (lower_bound <= self._result <= upper_bound):
            raise AssertionError('%s is not within the range [%s, %s]' % (self._result, lower_bound, upper_bound))
        
    def result_should_be(self, expected):
        """Check if value matches

        Args:
        expected(float): Expected value.

        """
        if self._result.value != expected:
            raise AssertionError('%s != %s' % (self._result, expected))
        
    def check_zmq_initialization(self):
        """Check ZMQ sockets intialization

        """
        if self._zmq_rc != 0:
            raise AssertionError('%s != %s' % (self._zmq_rc, 0))
        
    def check_i2c_devices_initialization(self):
        """Check I2C devices intialization

        """
        alarmas = []
        try:
            while True:
                mensaje = self.socket.recv_string(flags=zmq.NOBLOCK)
                mensaje_json = json.loads(mensaje)
                magnitudename = mensaje_json.get('magnitudename')
                if magnitudename:
                    alarma = {'magnitudename': magnitudename}
                    if 'channel' in mensaje_json:
                        alarma['channel'] = mensaje_json['channel']
                    else:
                        alarma['channel'] = ""
                    alarmas.append(alarma)
        except zmq.Again:
            self.socket.close()
            self.context.term()
            pass

        if alarmas:
            mensaje_error = 'The following alarms were triggered:\n'
            for alarma in alarmas:
                mensaje_error += f"Magnitude: {alarma['magnitudename']}"
                if 'channel' in alarma:
                    mensaje_error += f", Channel: {alarma['channel']}"
                mensaje_error += '\n'
            raise AssertionError(mensaje_error)

    def check_gpio_base_address(self):
        """Check GPIO Base Address is correct

        """
        if self.GPIO_Base_Address.value != 412:
            raise AssertionError('GPIO Base Address :%s != %s' % (self.GPIO_Base_Address, 412)) 
        
    def check_valid_command(self):
        """Check command is valid

        """
        if re.search(r'\bERROR\b', self.cmd_msg_value, re.IGNORECASE):
            raise AssertionError(f"An unexpected error was found in the command reply: {self.cmd_msg_value}")
    def check_set_command_error(self):
        """Check SET command error reply is correct

        """
        if not re.search(r'\bERROR: SET operation not successful\b', self.cmd_msg_value, re.IGNORECASE):
            raise AssertionError(f"An expected error was not found in the command reply: {self.cmd_msg_value}")
    def check_read_command_error(self):
        """Check READ command error reply is correct

        """
        if not re.search(r'\bERROR: READ operation not successful\b', self.cmd_msg_value, re.IGNORECASE):
            raise AssertionError(f"An expected error was not found in the command reply: {self.cmd_msg_value}")
    def check_invalid_command(self):
        """Check Invalid command error reply is correct

        """
        if not re.search(r'\bERROR: Command not valid\b', self.cmd_msg_value, re.IGNORECASE):
            raise AssertionError(f"An expected error was not found in the command reply: {self.cmd_msg_value}")
        
    def check_ethernet_alarm(self,eth_interface):
        """Forces and check Ethernet alarm triggered

        Args:
        eth_interface(string): Ethernet interface used to force alarm.

        """
        self.context = zmq.Context()
        self.socket = self.context.socket(zmq.SUB)
        self.socket.connect("tcp://127.0.0.1:5556")
        self.socket.setsockopt_string(zmq.SUBSCRIBE, "")

        if eth_interface == "Main" :
            c_interface = c_char_p("eth0")
            c_flag = "eth0_flag"
        elif eth_interface == "Backup":
            c_interface = c_char_p("eth1")
            c_flag = "eth1_flag"

        ethernet_flag = c_int.in_dll(self.dpb2sc, c_flag)
        self.set_ethernet_link_status(eth_interface,"ON")
        self.dpb2sc.eth_down_alarm(c_interface ,byref(ethernet_flag))
        self.set_ethernet_link_status(eth_interface,"OFF")
        self.dpb2sc.eth_down_alarm(c_interface ,byref(ethernet_flag))

        mensaje = self.socket.recv_string(flags=zmq.NOBLOCK)
        mensaje_json = json.loads(mensaje)
        magnitudename = mensaje_json.get('magnitudename')
        self.set_ethernet_link_status(c_interface ,"ON")
        self.socket.close()
        self.context.term()
        if re.search(r'\bBackup Ethernet Link Status\b', magnitudename, re.IGNORECASE):
            raise AssertionError(f"The expected alarm was not found in alarm socket, found this: {magnitudename}")
        
    def check_ams_voltage_alarm(self,channel):
        """Check AMS voltage alarm triggered after forcing it

        Args:
        channel(int): Channel that is supposed to trigger the alarm.

        """

        memory = Wrapper.in_dll(self.dpb2sc,"memory")
        memory_pointer = POINTER(memory)

        ev_type_str = memory_pointer.contents.ev_type.decode("utf-8")
        ch_type_str = memory_pointer.contents.ch_type.decode("utf-8")
        chann = memory_pointer.contents.chn
        if not (ev_type_str == "either") and (ch_type_str == "temp") and (chann == channel):
            raise AssertionError(f"The expected alarm was not detected by IIO Event Monitor")
        
    def check_sfp_presence(self):
        """Check all 6 SFPs presence

        """
        missing_sfps = []
        for i in range(6):
            self.read_gpio(13+(4*i)) 
            if self._result != 1:
                missing_sfps.append(i)
        if missing_sfps:
            mensaje_error = "The presence of the following SFPs has not been detected: "
            for sfp in missing_sfps:
                mensaje_error += f"SFP{sfp}, "
            mensaje_error = mensaje_error.rstrip(", ")  
            raise AssertionError(mensaje_error)

    def result_matches_rx_power_ranges(self):
        """Check SFP RX Power value is within manufacturer valid rangesm

        """
        lower_bound = 0.00002
        upper_bound = 0.001

        if not (lower_bound <= self._result <= upper_bound):
            raise AssertionError('%s is not within the RX Power range [%s, %s]' % (self._result, lower_bound, upper_bound))
        
    def result_matches_tx_power_ranges(self):
        """Check SFP TX Power value is within manufacturer valid rangesm

        """
        lower_bound = 0.000112
        upper_bound = 0.0005

        if not (lower_bound <= self._result <= upper_bound):
            raise AssertionError('%s is not within the TX Power range [%s, %s]' % (self._result, lower_bound, upper_bound))
        
    def check_sfp_gpio_pins (self,chip):
        """Check SFP GPIO pins value matches I2C corresponding register value

        Args:
        chip(string): SFP to be validated.

        """
        IntPointer = ctypes.POINTER(ctypes.c_int)
        int_array = (ctypes.c_int * 2)
        int_ptr = ctypes.cast(int_array, IntPointer)
        if chip == "SFP0":
            c_chip = c_int(0)
            i = 0
        elif chip == "SFP1": 
            c_chip = c_int(1)
            i = 1
        elif chip == "SFP2": 
            c_chip = c_int(2)
            i = 2
        elif chip == "SFP3": 
            c_chip = c_int(3)
            i = 3
        elif chip == "SFP4": 
            c_chip = c_int(4)
            i = 4
        elif chip == "SFP5": 
            c_chip = c_int(5)
            i = 5
        status = self.dpb2sc.sfp_avago_read_status(byref(self.structure_i2c),c_chip,int_ptr)
        self.read_gpio(14+(i*4))
        if (status & 0x02) and (self._result != 1):
            raise AssertionError('GPIO Pin RX_LOS value (%s) of SFP%s does not match I2C register value' % (self._result, i))
        self.read_gpio(6+i)
        if (status & 0x80) and (self._result != 1):
            raise AssertionError('GPIO Pin TX_Disable value (%s) of SFP%s does not match I2C register value' % (self._result, i))
        
    def check_iio_monitor(self):
    # Ejecuta el comando top -b -n 1 y captura la salida
        try:
            result = subprocess.run(['top', '-b', '-n', '1'], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            output = result.stdout

            # Busca la expresión IIO_MONITOR en la salida utilizando una expresión regular
            if not re.search(r'IIO_MONITOR -a /dev/iio:device0', output):
                raise AssertionError('Failed to run IIO Event Monitor')

        except Exception as e:
            print(f"Failed to execute command {e}")
            return 0
        
if __name__ == '__main__':
    RobotRemoteServer(DPB2scLibrary(), *sys.argv[1:])
    