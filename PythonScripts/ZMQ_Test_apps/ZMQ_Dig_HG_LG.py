import zmq
import json
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.dates as md
import sys
import dateutil
import datetime
import time

def main():
    if len(sys.argv) !=4:
        print("Number of arguments should be 3")
        print("1: IP address of the DPB \n2: Digitizer number \n3: Channel number")
        exit()
    
    if sys.argv[2] not in range(0,1):
        print("Digitizer number should be 0 or 1")

    if sys.argv[3] not in range(0,11):
        print("Channel number should be between 0 and 11")
    ip_str = "tcp://" + str(sys.argv[1]) + ":5557"
    
    dig_num = sys.argv[2]
    channel_num = sys.argv[3]
    context = zmq.Context()
    socket = context.socket(zmq.REQ)
    socket.connect(ip_str)
    print("Establishing connection with DPB at address " + str(str(sys.argv[1])))
    
    # Set Pedestal type to 4kHz
    cmd = "SET DIG" + dig_num + " PEDTYPE " + channel_num + " 2"
    send_command(socket,cmd)
    # Enable channel frontend
    cmd = "SET DIG" + dig_num + " FESTATUS " + channel_num + " ON"
    send_command(socket,cmd)
    
    LG = []
    HG = []
    datestrings  = []
    try:
        while True:
            cmd="READ DIG" + dig_num + " HG " + channel_num
            value = send_command(socket,cmd)
            HG.append(value)
            dig_num = sys.argv[2]
            channel_num = sys.argv[3]
            cmd="READ DIG" + dig_num + " LG " + channel_num
            value = send_command(socket,cmd)
            LG.append(value)
            datestrings.append(str(datetime.datetime.now()))
            time.sleep(0.4)
    except KeyboardInterrupt:
        plot_magnitude(datestrings,HG, "HG Charge")
        plot_magnitude(datestrings,LG, "LG Charge")
        
    
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
