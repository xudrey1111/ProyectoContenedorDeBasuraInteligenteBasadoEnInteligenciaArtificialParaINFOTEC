from static.ManejadorHTTP import Manejador 
from flask import Flask, render_template

app = Flask(__name__)
# Inicializa el manejador con la ruta del modelo de IA entrenado
#alpha = Manejador(modelo="C:\\Users\\XxGho\\OneDrive\\Documentos\\Escuela\\Proceso Dual\\Proyecto\\2° Proyecto\\Python\\Modelos\\Identificacion de images\\predictWaste_mobilenetv2.h5")
alpha=Manejador(modelo="C:\\Users\\XxGho\\OneDrive\\Documentos\\Escuela\\Proceso Dual\\Proyecto\\3° Proyecto\\Programacion\\static\\Modelos\\Identificacion de objetos\\yoloooo.pt")
"""
    Ruta principal que muestra la página web con los resultados de clasificación
    Returns:
        HTML: Página renderizada con los resultados de la clasificación
"""
@app.route('/index', methods=['GET'])
def index():
    if alpha.diccionarioIdentificacion is None:
        # Datos por defecto cuando no hay clasificación previa
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
    alpha.recepcionMensaje()
    return alpha.enviarMensaje()

"""
    Punto de entrada principal que inicia el servidor Flask
"""
if __name__ == '__main__':
    try:
        # Inicia el servidor con la IP obtenida dinámicamente en el puerto 5000
        app.run(host=alpha.getIpServidor(), port=5000, debug=False)
    except Exception as e:
        print(f"Error al iniciar el servidor: {e}")