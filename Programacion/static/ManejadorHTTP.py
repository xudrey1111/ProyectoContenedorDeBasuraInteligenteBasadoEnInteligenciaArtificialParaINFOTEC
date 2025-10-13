import base64
from io import BytesIO
from flask import Flask, request, jsonify
from static.pruebaModelos import *
import socket

class Manejador():
    def __init__(self, modelo):
        self.data=None
        self.imagen=None
        self.bravo=pruebaModeloIA(modelo)

    def recepcionMensaje(self):
        if not request.is_json:
            print("Servidor: Error - La solicitud no contiene formato JSON.")
            return jsonify({"status": "error", "message": "Se espera JSON"}), 400

        self.data = request.get_json()
        return self.data
    
    def enviarMensaje(self):
        if self.data.get("evento") == "objeto identificado":
            print("Servidor: 'Objeto identificado' recibido. Iniciando clasificación...")
            imagen_b64 = self.data.get("imagen_b64")
            if not imagen_b64:
                print("Servidor: Error - El JSON no contiene el campo 'imagen_b64'.")
                return jsonify({"status": "error", "message": "Falta la imagen Base64"}), 400
            try:
                imagen_bytes = base64.b64decode(imagen_b64)
                print(f"Servidor: Imagen decodificada, tamaño de bytes JPEG: {len(imagen_bytes)}")
                self.imagen = BytesIO(imagen_bytes)
                procesarImagen = self.bravo.processImage(self.imagen)
                getNombreClase = self.bravo.predictImage(procesarImagen).get('clase', '')
                clasificacion = "N" if getNombreClase == "No Biodegradable" else "B"
                response_data = {"status": "ok", "clasificacion": clasificacion}
                print(f"→ Enviando al ESP32 (Respuesta HTTP 200): {response_data}")
                return jsonify(response_data), 200
            except Exception as e:
                print(f"Servidor: Error al procesar la imagen: {e}")
                return jsonify({"status": "error", "message": f"Error al procesar la imagen: {str(e)}"}), 500
        else:
            print(f"Servidor: Se esperaba 'objeto identificado' pero se recibió el evento: {self.data.get('evento', 'N/A')}")
            return jsonify({"status": "error", "message": "Evento no reconocido"}), 400

    def getIpServidor(self):
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            s.connect(("8.8.8.8", 80))
            ip = s.getsockname()[0]
            s.close()
            return ip
        except Exception:
            return "127.0.0.1 (Verifica tu IP)"