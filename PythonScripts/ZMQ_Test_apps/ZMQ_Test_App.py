import zmq
import json
import numpy as np
import matplotlib.pyplot as plt

temp_val = np.tile(np.float32(0),20)
timestamp = np.tile(0,20)
init_timestamp = np.tile(0,1)


def process_json(json_data,i):
    # Parsear el JSON
    json_obj = json.loads(json_data)
    if i == 0 :
        init_timestamp[0] = json_obj['timestamp']
        timestamp[0] = 0
    else:
        timestamp[i] = json_obj['timestamp'] - init_timestamp[0]
    # Extraer información de cada campo
    data = json_obj['data']

    # # Extraer información de los subcampos de 'data'
    dpb_data = data['DPB']
    pl_temp = dpb_data[10]
    temp_val[i] = pl_temp['value']
    print(pl_temp['value'])
    
    # # Imprimir la información extraída
    print(json.dumps(json_obj, indent=4))

def main():
    context = zmq.Context()
    socket = context.socket(zmq.SUB)
    
    socket.setsockopt_string(zmq.SUBSCRIBE, "")
    socket.connect("tcp://20.0.0.30:5555")
    
    
    for i in range(0, 20):
        json_data = socket.recv_string()
        #print(json_data)
        process_json(json_data,i)
    plt.plot(timestamp,temp_val)
    plt.title("PL Temperature")
    plt.xlabel("Time (seconds)")
    plt.ylabel("Temperature (ºC)")
    plt.ylim(70, 81)
    plt.show()

if __name__ == "__main__":
    main()
