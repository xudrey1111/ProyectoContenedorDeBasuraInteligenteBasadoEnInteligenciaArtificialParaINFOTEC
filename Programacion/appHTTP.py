import time
from static.ManejadorHTTP import Manejador 
from flask import Flask, jsonify, render_template, request

app = Flask(__name__)
# Inicializa el manejador con la ruta del modelo de IA entrenado
alpha = Manejador(modelo="C:\\Users\\XxGho\\OneDrive\\Documentos\\Escuela\\Proceso Dual\\Proyecto\\3° Proyecto\\Programacion\\static\\Modelos\\Identificacion de images\\model_retrained_REALDATA.h5")
#alpha=Manejador(modelo="C:\\Users\\XxGho\\OneDrive\\Documentos\\Escuela\\Proceso Dual\\Proyecto\\3° Proyecto\\Programacion\\static\\Modelos\\Identificacion de objetos\\yoloooo.pt")

ultima_actualizacion = 0


"""
    Ruta principal que muestra la página web con los resultados de clasificación
    Returns:
        HTML: Página renderizada con los resultados de la clasificación
"""
@app.route('/index', methods=['GET'])
def index():
    if alpha.diccionarioIdentificacion is None:
        resultado_default = {
            'clase': 'Esperando detección...',
            'probabilidad': 0.0,
            'imagen_path': 'Imagenes/placeholder.jpg'
        }
        return render_template('index.html', resultado=resultado_default)
    return render_template('index.html', resultado=alpha.diccionarioIdentificacion)

"""
    Endpoint para recibir y procesar solicitudes de clasificación de objetos
    Returns:
        tuple: Respuesta JSON con el resultado de la clasificación y código de estado
"""
@app.route('/', methods=['POST'])
def clasificar_objeto():
    global ultima_actualizacion
    alpha.recepcionMensaje()
    respuesta = alpha.enviarMensaje()
    ultima_actualizacion = time.time()
    return respuesta

"""
    Endpoint para recibir y procesar solicitudes de clasificación de objetos
    Returns:
        tuple: Respuesta JSON con el resultado de la clasificación y código de estado
"""
@app.route('/verificar_actualizacion', methods=['GET'])
def verificar_actualizacion():
    global ultima_actualizacion
    timestamp_cliente = float(request.args.get('timestamp', 0))
    timeout = 30  # Segundos máximos de espera antes de cerrar conexión para evitar timeout del navegador
    inicio = time.time()
    while True:
        if ultima_actualizacion > timestamp_cliente:
            if (alpha.diccionarioIdentificacion and 
                alpha.diccionarioIdentificacion.get('clase') != 'Esperando detección...'):
                
                return jsonify({
                    'actualizado': True,
                    'timestamp': ultima_actualizacion,
                    'datos': alpha.diccionarioIdentificacion
                })
        if (time.time() - inicio) > timeout:
            return jsonify({
                'actualizado': False,
                'timestamp': ultima_actualizacion
            })
        time.sleep(0.5)

"""
    Punto de entrada principal que inicia el servidor Flask
"""
if __name__ == '__main__':
    try:
        # Inicia el servidor con la IP obtenida dinámicamente en el puerto 5000
        # En tu bloque if __name__ == '__main__':
        app.run(host=alpha.getIpServidor(), port=5000, debug=False, threaded=True)
    except Exception as e:
        print(f"Error al iniciar el servidor: {e}")