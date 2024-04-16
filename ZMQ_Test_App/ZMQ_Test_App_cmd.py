import zmq
import numpy as np
import matplotlib.pyplot as plt

temp_val = np.tile(np.float32(0),10)
timestamp = np.tile(0,10)
init_timestamp = np.tile(0,1)


# def process_json(json_data,i):
#     # Parsear el JSON
#     json_obj = json.loads(json_data)
#     if i == 0 :
#         init_timestamp[0] = json_obj['timestamp']
#         timestamp[0] = 0
#     else:
#         timestamp[i] = json_obj['timestamp'] - init_timestamp[0]
#     # Extraer información de cada campo
#     data = json_obj['data']

#     # # Extraer información de los subcampos de 'data'
#     dpb_data = data['DPB']
#     pl_temp = dpb_data[10]
#     temp_val[i] = pl_temp['value']
#     print(pl_temp['value'])
    
#     # # Imprimir la información extraída
#     #print(json.dumps(json_obj, indent=4))

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
