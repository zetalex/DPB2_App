import zmq
import time
import sys
import json


def main():
    
    ip_str = "tcp://" + str(sys.argv[1]) + ":5557"
    
    context = zmq.Context()
    socket = context.socket(zmq.REQ)
    socket.connect(ip_str)
    print("Establishing connection with DPB at address " + str(str(sys.argv[1])))

    while True:
        command = input("Type a command: ")
        msg = "{'msg_id':0, 'msg_time':'2021-11-19T17:54:30.691Z', 'msg_type':'Command', 'msg_value':'" + command + "', 'uuid': '931fbc9d-b2b3-c248-87d6ae33f9a62'}"
        socket.send_string(msg)
        response = socket.recv_string()
        json_obj = json.loads(response)
        value = json_obj["msg_value"]
        print(value)
    

if __name__ == "__main__":
    main()
    
# Examples of request-reply JSON
#{"msg_id":0, "msg_time":"2021-11-19T17:54:30.691Z", "msg_type":"Command", "msg_value":"READ DPB TEMP PCB", "uuid": "931fbc9d-b2b3-c248-87d6ae33f9a62"}
    
#{ "msg_id": 0, "msg_time": "2021-11-19T17:54:30.691Z", "msg_type": "Command reply", "msg_value": 38.5000, "uuid": "931fac9d-b2b3-c248-87d6ae33f9a62" }