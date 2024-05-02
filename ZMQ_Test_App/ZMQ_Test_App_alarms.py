import zmq
import json



def process_json(json_data):
    # Parsear el JSON
    json_obj = json.loads(json_data)
    
    # # Imprimir la información extraída
    print(json.dumps(json_obj, indent=4))

def main():
    context = zmq.Context()
    socket = context.socket(zmq.SUB)
    
    socket.setsockopt_string(zmq.SUBSCRIBE, "")
    socket.connect("tcp://20.0.0.33:5556")
    
    
    while True:
        json_data = socket.recv_string()
        #print(json_data)
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