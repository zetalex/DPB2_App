import zmq
import time


def main():
    context = zmq.Context()
    socket = context.socket(zmq.REQ)
    socket.connect("tcp://20.0.0.30:5557")

    print("Hola")
    while True:
        #message = input("Ingrese un mensaje: ")
        msg = "{'msg_id':0, 'msg_time':'2021-11-19T17:54:30.691Z', 'msg_type':'Command', 'msg_value':'READ DPB TEMP PCB', 'uuid': '931fbc9d-b2b3-c248-87d6ae33f9a62'}"
        socket.send_string(msg)
        response = socket.recv_string()
        print(response)
        time.sleep(1)
    

if __name__ == "__main__":
    main()
#{"msg_id":0, "msg_time":"2021-11-19T17:54:30.691Z", "msg_type":"Command", "msg_value":"READ DPB TEMP PCB", "uuid": "931fbc9d-b2b3-c248-87d6ae33f9a62"}
    
#{ "msg_id": 0, "msg_time": "2021-11-19T17:54:30.691Z", "msg_type": "Command reply", "msg_value": 38.5000, "uuid": "931fac9d-b2b3-c248-87d6ae33f9a62" }