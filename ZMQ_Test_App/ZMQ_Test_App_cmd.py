import zmq


def main():
    context = zmq.Context()
    socket = context.socket(zmq.REQ)
    socket.connect("tcp://20.0.0.33:5557")

    print("Hola")
    while True:
        message = input("Ingrese un mensaje: ")
        socket.send_string(message)
        response = socket.recv_string()
        print(response)
    

if __name__ == "__main__":
    main()
#{"msg_id":0, "msg_time":"2021-11-19T17:54:30.691Z", "msg_type":"Command", "msg_value":"READ DPB TEMP PCB", "uuid": "931fac9d-b2b3-c248-87d6ae33f9a62"}
    
#{ "msg_id": 0, "msg_time": "2021-11-19T17:54:30.691Z", "msg_type": "Command reply", "msg_value": 38.5000, "uuid": "931fac9d-b2b3-c248-87d6ae33f9a62" }