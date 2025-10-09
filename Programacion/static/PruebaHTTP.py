from flask import Flask, request, jsonify

app = Flask(__name__)

@app.route('/upload', methods=['POST'])
def upload():
    """
    Maneja la recepción de datos JSON enviados por el ESP32.
    """
    if not request.is_json:
        return jsonify({"status": "error", "message": "Content-Type debe ser application/json"}), 415

    try:
        data = request.get_json()
        
        print("--- Petición POST JSON recibida ---")
        print(f"Estado: {data.get('estado', 'N/A')}")
        print(f"Mensaje: {data.get('mensaje', 'N/A')}")
        print(f"Dispositivo ID: {data.get('dispositivo_id', 'N/A')}")
        print("-----------------------------------")

        response = {
            "status": "recibido",
            "mensaje": "Mensaje JSON procesado correctamente por el servidor"
        }
        
        return jsonify(response), 200

    except Exception as e:
        print(f"Error al procesar JSON o interno del servidor: {e}")
        return jsonify({'status': 'error', 'message': f'Error al procesar los datos: {str(e)}'}), 500

if __name__ == '__main__':
    print("Iniciando servidor Flask en http://0.0.0.0:5000/upload. Presiona Ctrl+C para detener.")
    app.run(host='0.0.0.0', port=5000, debug=True)
