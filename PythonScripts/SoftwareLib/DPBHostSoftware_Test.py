import DPBHostSoftware
dpb_ip = "20.0.0.33"
monitoring_port = 5555
cmd_port = 5557
config_port = 5559
data_port = 5570
logging_port = 5558
# Create an instance of the DPBHostSoftware class
instance = DPBHostSoftware.DPBHostSoftware(dpb_ip, monitoring_port, cmd_port, config_port, logging_port, data_port,"dpb_logging.txt")
# Get a command value
command_value = instance.send_slow_control_command("READ DPB TEMP PCB")
print(f"PCB Temperature value on the DPB: {command_value} ºC")
# Get a monitoring JSON
monitoring_data = instance.get_mon_data()
print(f"Monitoring Data: {monitoring_data}")

# Use Digitizers as data source
return_set= instance.send_slow_control_command("SET DPB DMASOURCE DIG")
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