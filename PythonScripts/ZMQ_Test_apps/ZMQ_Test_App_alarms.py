import zmq
import json
import sys


def process_json(json_data):
    # Parse JSON
    json_obj = json.loads(json_data)
    
    # Print extracted info
    print(json.dumps(json_obj, indent=4))

def main():
    ip_str = "tcp://" + str(sys.argv[1]) + ":5556"
    
    context = zmq.Context()
    socket = context.socket(zmq.SUB)
    
    socket.setsockopt_string(zmq.SUBSCRIBE, "")
    socket.connect(ip_str)
    
    print("Establishing connection with DPB at address " + str(str(sys.argv[1])))
    
    while True:
        json_data = socket.recv_string()
        process_json(json_data)


if __name__ == "__main__":
    main()

# {
#     "board": "DPB",
#     "magnitudename": "SFP RX Power",
#     "eventtype": "falling",
#     "eventtimestamp": 1637343635000,
#     "channel": 0,
#     "value": 0.0
# }
# {
#     "board": "DPB",
#     "magnitudename": "Backup Ethernet Link Status",
#     "eventtimestamp": 1637343636000,
#     "value": "OFF"
# }