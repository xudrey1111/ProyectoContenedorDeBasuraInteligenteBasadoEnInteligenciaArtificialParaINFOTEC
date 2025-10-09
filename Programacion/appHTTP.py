from static.ManejadorHTTP import Manejador 
from flask import Flask, render_template

app = Flask(__name__)
imagen="C:\\Users\\XxGho\\OneDrive\\Documentos\\Escuela\\Proceso Dual\\Proyecto\\2° Proyecto\\Dataset personalizado\\test\\N\\nobiodegradable0012.jpg"
alpha = Manejador(imagen,modelo="C:\\Users\\XxGho\\OneDrive\\Documentos\\Escuela\\Proceso Dual\\Proyecto\\2° Proyecto\\Python\\Modelos\\Identificacion de images\\predictWaste_mobilenetv2.h5")

@app.route('/index', methods=['GET'])
def index():
    simulated_result = {
        'clase': 'No Biodegradable', # Nombre de clase del modelo
        'etiqueta': 'N',              # Etiqueta corta que se enviaría al ESP32
    }
    return render_template('index.html', imagen_path=imagen, resultado=simulated_result)

@app.route('/', methods=['POST'])
def clasificar_objeto():
    datos_recibidos = alpha.recepcionMensaje()
    return alpha.enviarMensaje()

if __name__ == '__main__':
    ip_servidor = alpha.getIpServidor() 
    
    print("--- Servidor Flask iniciado ---")
    print(f"Dirección local: http://{ip_servidor}:5000/")
    try:
        app.run(host='0.0.0.0', port=5000, debug=False)
    except Exception as e:
        print(f"Error al iniciar el servidor: {e}")
