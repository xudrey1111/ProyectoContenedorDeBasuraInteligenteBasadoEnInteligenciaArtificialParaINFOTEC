import base64
from io import BytesIO

from pruebaModelos import *
from flask import Flask, request, jsonify

app = Flask(__name__)
modelo="C:\\Users\\XxGho\\OneDrive\\Documentos\\Escuela\\Proceso Dual\\Proyecto\\3° Proyecto\\Programacion\\static\\Modelos\\Identificacion de images\\predictWaste_mobilenetv2.h5"
bravo=pruebaModeloIA(modelo)

@app.route('/', methods=['POST'])
def clasificar_objeto():
    if not request.is_json:
        print("Servidor: Error - La solicitud no contiene formato JSON.")
        return jsonify({"status": "error", "message": "Se espera JSON"}), 400
    
    data = request.get_json()
    if data.get("evento") == "objeto identificado":
        print("Servidor: 'Objeto identificado' recibido. Iniciando clasificación...")
        imagen_b64 = data.get("imagen_b64")
        
        if not imagen_b64:
            print("Servidor: Error - El JSON no contiene el campo 'imagen_b64'.")
            return jsonify({"status": "error", "message": "Falta la imagen Base64"}), 400
        
        try:
            imagen_bytes = base64.b64decode(imagen_b64)
            print(f"Servidor: Imagen decodificada, tamaño de bytes JPEG: {len(imagen_bytes)}")
            imagen_io = BytesIO(imagen_bytes)

            procesarImagen = bravo.processImage(imagen_io)
            getNombreClase = bravo.predictImage(procesarImagen).get('clase', '')
            clasificacion = "N" if getNombreClase == "No Biodegradable" else "B"
            
            response_data = {"status": "ok", "clasificacion": clasificacion}
            print(f"→ Enviando al ESP32 (Respuesta HTTP 200): {response_data}")
            return jsonify(response_data), 200
            
        except Exception as e:
            print(f"Servidor: Error al procesar la imagen: {e}")
            return jsonify({"status": "error", "message": f"Error al procesar la imagen: {str(e)}"}), 500
    else:
        print(f"Servidor: Se esperaba 'objeto identificado' pero se recibió el evento: {data.get('evento', 'N/A')}")
        return jsonify({"status": "error", "message": "Evento no reconocido"}), 400

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=False)