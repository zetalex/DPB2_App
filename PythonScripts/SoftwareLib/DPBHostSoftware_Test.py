import DPBHostSoftware
import time

dpb_ip = "20.0.0.33"
monitoring_port = 5555
cmd_port = 5557
config_port = 5559
data_port = 5570
logging_port = 5558
alarm_port = 5556
# Create an instance of the DPBHostSoftware class
instance = DPBHostSoftware.DPBHostSoftware(dpb_ip, monitoring_port, alarm_port, cmd_port, config_port, logging_port, data_port,"dpb_logging.txt")
# Get a command value
command_value = instance.send_slow_control_command("READ DPB TEMP PCB")
print(f"PCB Temperature value on the DPB: {command_value} ºC")
# Get a monitoring JSON
monitoring_data = instance.get_mon_data()
print(f"Monitoring Data: {monitoring_data}")

# Turn off one ethernet interface using ifconfig command in the PC
system_command = "/sbin/ifconfig eth1 down"
return_value = instance.send_ssh_command(system_command)
print(f"SSH Command '{system_command}' returned: {return_value}")
time.sleep(2)
# Turn on again
system_command = "/sbin/ifconfig eth1 up"
return_value = instance.send_ssh_command(system_command)
print(f"SSH Command '{system_command}' returned: {return_value}")

# Use Digitizers as data source
return_set= instance.send_slow_control_command("SET DPB DMASOURCE COUNTER")
if return_set != "OK":
    print("Error setting DMASOURCE to DIG")
# Get 4 seconds of data
data = instance.get_dig_data(4000)
with open("digital_data_4s.bin", 'wb') as f:
    f.write(data)
    f.flush()
    f.close()

# Clean up
del instance
