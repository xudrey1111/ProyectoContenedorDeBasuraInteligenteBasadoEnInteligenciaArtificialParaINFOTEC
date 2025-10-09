from static.ManejadorHTTP import Manejador 
from flask import Flask 

app = Flask(__name__)
alpha = Manejador()

@app.route('/', methods=['POST'])
def clasificar_objeto():
    datos_recibidos = alpha.recepcionMensaje()
    return alpha.enviarMensaje()

if __name__ == '__main__':
    ip_servidor = alpha.getIpServidor() 
    
    print("--- Servidor Flask iniciado ---")
    print(f"Dirección local: http://{ip_servidor}:5000/")
    print(f"Esperando clientes del ESP32 en el endpoint: /")
    try:
        app.run(host='0.0.0.0', port=5000, debug=False)
    except Exception as e:
        print(f"Error al iniciar el servidor: {e}")
