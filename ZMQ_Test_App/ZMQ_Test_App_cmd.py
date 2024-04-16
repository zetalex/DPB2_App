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
