import zmq
import time
import sys
import json
from datetime import datetime

def send_command(socket, command):
    timestamp = datetime.utcnow().isoformat() + "Z"

    msg = json.dumps({
        "msg_id": 0,
        "msg_time": timestamp,
        "msg_type": "Command",
        "msg_value": command,
        "uuid": "931fbc9d-b2b3-c248-87d6ae33f9a62"
    })

    print(f"Sending: {command}")
    socket.send_string(msg)

    response = socket.recv_string()
    json_obj = json.loads(response)
    value = json_obj.get("msg_value", "No msg_value in response")
    print(f"Response: {value}")

def main():
    ip_str = "tcp://" + str(sys.argv[1]) + ":5557"

    context = zmq.Context()
    socket = context.socket(zmq.REQ)
    socket.connect(ip_str)
    print("Establishing connection with DPB at address " + str(sys.argv[1]))

    for numero1 in range(12):
        cmd_dig0 = f"SET DIG0 PEDTYPE {numero1} 1"
        cmd_dig1 = f"SET DIG1 PEDTYPE {numero1} 1"

        send_command(socket, cmd_dig0)
        time.sleep(0.5)
        
        send_command(socket, cmd_dig1)
        time.sleep(0.5)

    cmd_dig0_daq=f"SET DIG0 DAQSTATUS ALL ON"
    cmd_dig1_daq=f"SET DIG1 DAQSTATUS ALL ON"
    send_command(socket, cmd_dig0_daq)
    time.sleep(0.5)

    send_command(socket, cmd_dig1_daq)
    time.sleep(0.5)
if __name__ == "__main__":
    main()
