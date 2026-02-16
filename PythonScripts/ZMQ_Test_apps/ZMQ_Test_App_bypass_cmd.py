import zmq
import time
import sys
import json


def main():
    
    ip_str = "tcp://" + str(sys.argv[1]) + ":5560"
    
    context = zmq.Context()
    socket = context.socket(zmq.REQ)
    socket.connect(ip_str)
    print("Establishing connection with DPB at address " + str(str(sys.argv[1])))

    while True:
        command = input("Type a command: ")
        tstart = time.time()
        socket.send_string(command)
        response = socket.recv_string()
        tend = time.time()
        print(f"Response time: {(tend - tstart) * 1000} ms")
        print(response)


if __name__ == "__main__":
    main()
    
# Examples of request-reply JSON
# DIG0 $ggwv#
# DIG1 $ggwv#
