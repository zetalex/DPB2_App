import zmq
import json
import sys
import threading
import queue
import time


def zmq_receiver_thread(ip_str, message_queue):
    """Hilo para recibir mensajes ZMQ"""
    context = zmq.Context()
    socket = context.socket(zmq.SUB)
    
    socket.setsockopt_string(zmq.SUBSCRIBE, "")
    socket.connect(ip_str)
    
    print("Establishing connection with DPB at address " + ip_str)
    
    try:
        while True:
            # Recibir mensaje sin bloquear por mucho tiempo
            try:
                log_data = socket.recv_string(zmq.NOBLOCK)
                message_queue.put(log_data)
            except zmq.Again:
                # No hay mensajes disponibles, continuar
                time.sleep(0)  # Pequeña pausa para no consumir demasiada CPU
    except KeyboardInterrupt:
        pass
    finally:
        socket.close()
        context.term()


def printer_thread(message_queue):
    """Hilo para imprimir mensajes"""
    try:
        while True:
            try:
                # Obtener mensaje de la cola con timeout
                log_data = message_queue.get(timeout=1)
                
                # Detectar si el mensaje es JSON
                is_json = False
                try:
                    json.loads(log_data)
                    is_json = True
                except (json.JSONDecodeError, ValueError):
                    is_json = False
                
                # Imprimir con formato según el tipo
                if is_json:
                    print(log_data)  # JSON con salto de línea normal
                else:
                    print(log_data, end="")  # No JSON sin salto de línea extra
                
                message_queue.task_done()
            except queue.Empty:
                # No hay mensajes en la cola, continuar
                continue
    except KeyboardInterrupt:
        pass


def main():
    if len(sys.argv) < 2:
        print("Uso: python ZMQ_Test_App_logging.py <IP_ADDRESS>")
        sys.exit(1)
        
    ip_str = "tcp://" + str(sys.argv[1]) + ":5558"
    
    # Crear cola para comunicación entre hilos
    message_queue = queue.Queue(maxsize=1000)  # Limitar tamaño para evitar uso excesivo de memoria
    
    # Crear y iniciar hilos
    receiver_thread = threading.Thread(target=zmq_receiver_thread, args=(ip_str, message_queue))
    printer_thread_obj = threading.Thread(target=printer_thread, args=(message_queue,))
    
    # Configurar hilos como daemon para que terminen cuando el programa principal termine
    receiver_thread.daemon = True
    printer_thread_obj.daemon = True
    
    # Iniciar hilos
    receiver_thread.start()
    printer_thread_obj.start()
    
    try:
        # Mantener el programa principal activo
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print("\nCerrando aplicación...")
        sys.exit(0)


if __name__ == "__main__":
    main()