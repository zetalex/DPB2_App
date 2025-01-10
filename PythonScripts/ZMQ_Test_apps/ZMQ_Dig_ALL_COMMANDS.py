import zmq
import json
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.dates as md
import sys
import dateutil
import Dig_commands
import datetime
import time

def main():
    if len(sys.argv) !=2:
        print("Number of arguments should be 1")
        print("1: IP address of the DPB\n")
        exit()
    
    ip_str = "tcp://" + str(sys.argv[1]) + ":5557"
    
    context = zmq.Context()
    socket = context.socket(zmq.REQ)
    socket.connect(ip_str)
    nerrors = 0
    print("Establishing connection with DPB at address " + str(str(sys.argv[1])))
    
    # For loop
    print("Doing environment variables read")
    for cmd in Dig_commands.dig_dpb_environ_commands_read:
       print(cmd)
       response = send_command(socket,cmd)
       print(response)
       time.sleep(0.2)
       response = str(response)
       if(response.startswith("ERROR")):
           nerrors= nerrors + 1
    
    print("Doing environment variables set")
    for cmd in Dig_commands.dig_dpb_environ_commands_set:
       print(cmd)
       response = send_command(socket,cmd)
       print(response)
       time.sleep(0.2)
       response = str(response)
       if(response.startswith("ERROR")):
           nerrors= nerrors + 1
    
    print("Doing channel variables read")
    for cmd in Dig_commands.dig_dpb_channel_commands_read:
       print(cmd)
       response = send_command(socket,cmd)
       print(response)
       time.sleep(0.2)
       response = str(response)
       if(response.startswith("ERROR")):
           nerrors= nerrors + 1
       
    print("Doing channel variables set")
    for cmd in Dig_commands.dig_dpb_channel_commands_set:
       print(cmd)
       response = send_command(socket,cmd)
       print(response)
       time.sleep(0.2)
       response = str(response)
       if(response.startswith("ERROR")):
           nerrors= nerrors + 1
       
    print("DONE. Number of commands failed:" + str(nerrors))
        
    
def send_command(socket,cmd):
    msg = "{'msg_id':0, 'msg_time':'2021-11-19T17:54:30.691Z', 'msg_type':'Command', 'msg_value':'" + cmd + "', 'uuid': '931fbc9d-b2b3-c248-87d6ae33f9a62'}"
    socket.send_string(msg)
    response = socket.recv_string()
    json_obj = json.loads(response)
    value = json_obj["msg_value"]
    return value

def plot_magnitude(x,y,title):
    dates = [dateutil.parser.parse(s) for s in x]

    plt_data = range(5,9)
    plt.subplots_adjust(bottom=0.2)
    plt.xticks( rotation=25 )

    ax=plt.gca()
    ax.set_xticks(dates)

    xfmt = md.DateFormatter('%Y-%m-%d %H:%M:%S')
    ax.xaxis.set_major_formatter(xfmt)
    plt.title(title)
    plt.xlabel("Time (date)")
    plt.ylabel("Magnitude (units)")
    plt.plot(dates,y, "o-")
    plt.show()
    
if __name__ == "__main__":
    main()
    
# Examples of request-reply JSON
#{"msg_id":0, "msg_time":"2021-11-19T17:54:30.691Z", "msg_type":"Command", "msg_value":"READ DPB TEMP PCB", "uuid": "931fbc9d-b2b3-c248-87d6ae33f9a62"}
    
#{ "msg_id": 0, "msg_time": "2021-11-19T17:54:30.691Z", "msg_type": "Command reply", "msg_value": 38.5000, "uuid": "931fac9d-b2b3-c248-87d6ae33f9a62" }
