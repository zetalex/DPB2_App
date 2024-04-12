import zmq
import json

def process_json(json_data):
    # Parsear el JSON
    json_obj = json.loads(json_data)

    # Extraer información de cada campo
    timestamp = json_obj['timestamp']
    device_id = json_obj['device']
    data = json_obj['data']

    # # Extraer información de los subcampos de 'data'
    # lv_data = data['LV']
    # hv_data = data['HV']
    # dig0_data = data['Dig0']
    # dig1_data = data['Dig1']
    # dpb_data = data['DPB']

    # # Imprimir la información extraída
    # print("Timestamp:", timestamp)
    # print("Device ID:", device_id)
    # print("LV Data:", lv_data)
    # print("HV Data:", hv_data)
    # print("Dig0 Data:", dig0_data)
    # print("Dig1 Data:", dig1_data)
    # print("DPB Data:", dpb_data)
    print(json.dumps(json_obj, indent=4))

def main():
    context = zmq.Context()
    socket = context.socket(zmq.SUB)
    
    socket.setsockopt_string(zmq.SUBSCRIBE, "")
    socket.connect("tcp://20.0.0.33:5555")
    
    
    while True:
        print("Holita 2")
        json_data = socket.recv_string()
        print(json_data)
        print("Holita")
        process_json(json_data)

if __name__ == "__main__":
    main()
