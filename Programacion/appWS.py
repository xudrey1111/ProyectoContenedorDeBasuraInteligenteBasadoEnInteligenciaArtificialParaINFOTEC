from flask import Flask, render_template
from flask_socketio import SocketIO, emit

app = Flask(__name__)
app.config['SECRET_KEY'] = 'mi_clave_secreta'
socketio = SocketIO(app, cors_allowed_origins="*",logger=True,engineio_logger=True)

# --- Lógica del WebSocket ---

@socketio.on('connect')
def handle_connect():
    print('Cliente ESP32 conectado.')

@socketio.on('disconnect')
def handle_disconnect():
    print('Cliente ESP32 conectado.')

@socketio.on('message')
def handle_message(data):
    print(f"Mensaje recibido del ESP32: {data}")

    response_message = "Servidor: Mensaje recibido. Enviando respuesta al ESP32."
    emit('response_to_client', {'server_message': response_message}) 

@socketio.on('client_confirmation')
def handle_confirmation(data):
    print(f"CONFIRMACIÓN RECIBIDA del ESP32: {data.get('status')}")

if __name__ == '__main__':
    socketio.run(app, host='0.0.0.0', port=5000)