from static.ManejadorHTTP import Manejador 
from flask import Flask, render_template

app = Flask(__name__)
alpha = Manejador(modelo="C:\\Users\\XxGho\\OneDrive\\Documentos\\Escuela\\Proceso Dual\\Proyecto\\2° Proyecto\\Python\\Modelos\\Identificacion de images\\predictWaste_mobilenetv2.h5")

@app.route('/index', methods=['GET'])
def index():
    if alpha.diccionarioIdentificacion is None:
        resultado_default = {
            'clase': 'Esperando detección...',
            'probabilidad': 'N/A',
            'imagen_path': 'Programacion/Static/Imagenes/placeholder.jpg'
        }
        return render_template('index.html', resultado=resultado_default)
    return render_template('index.html', resultado=alpha.diccionarioIdentificacion)

@app.route('/', methods=['POST'])
def clasificar_objeto():
    alpha.recepcionMensaje()
    return alpha.enviarMensaje()

if __name__ == '__main__':
    try:
        app.run(host=alpha.getIpServidor(), port=5000, debug=True)
    except Exception as e:
        print(f"Error al iniciar el servidor: {e}")