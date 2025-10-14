from datetime import datetime
from io import BytesIO
import uuid
from flask import Flask, request, jsonify
from static.pruebaModelos import *
import socket

class Manejador():
    def __init__(self, modelo):
        self.data=None
        self.imagen=None
        self.diccionarioIdentificacion=None
        self.bravo=pruebaModeloIA(modelo)
        self.imagenes_dir = os.path.join('Programacion','static', 'imagenes')
        self.ultimaImagenCapturada = None

    def recepcionMensaje(self):
        if not request.is_json:
            print("Servidor: Error - La solicitud no contiene formato JSON.")
            return jsonify({"status": "error", "message": "Se espera JSON"}), 400

        self.data = request.get_json()
        return self.data
    
    def enviarMensaje(self):
        evento = self.data.get("evento")
        if evento == "objeto identificado":
            print(f"Servidor: Objeto identificado iniciando modelo de ia")
            return jsonify({"status": "ok", "message": "Notificación recibida"}), 200
        elif evento == "imagen bytes":
            imagen_bytes_array = self.data.get("imagen_bytes")
            if not imagen_bytes_array:
                return jsonify({"status": "error", "message": "Falta el array de bytes de la imagen"}), 400
            try:
                imagen_bytes = bytes(imagen_bytes_array)
                print(f"Servidor: Imagen recibida como bytes - tamaño: {len(imagen_bytes)} bytes")
                imagen_path = self.guardarImagen(imagen_bytes)
                self.ultimaImagenCapturada = imagen_path
                self.imagen = BytesIO(imagen_bytes)
                procesarImagen = self.bravo.processImage(self.imagen)
                self.diccionarioIdentificacion = self.bravo.predictImage(procesarImagen)
                if imagen_path:
                    nombre_archivo = os.path.basename(imagen_path)
                    ruta_web = f"imagenes/{nombre_archivo}"  # Usar siempre / para web
                    self.diccionarioIdentificacion['imagen_path'] = ruta_web
                getNombreClase = self.diccionarioIdentificacion.get('clase', '')
                clasificacion = "N" if getNombreClase == "No Biodegradable" else "B"
                response_data = {
                    "status": "ok", 
                    "clasificacion": clasificacion,
                    "clase_detectada": getNombreClase
                }
                return jsonify(response_data), 200
            except Exception as e:
                print(f"Servidor: Error al procesar la imagen: {str(e)}")
                return jsonify({"status": "error", "message": f"Error al procesar la imagen: {str(e)}"}), 500
        else:
            return jsonify({"status": "error", "message": "Evento no reconocido"}), 400

    def getIpServidor(self):
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            s.connect(("8.8.8.8", 80))
            ip = s.getsockname()[0]
            s.close()
            return ip
        except Exception:
            return "0.0.0.0"
        
    def guardarImagen(self, imagen_bytes):
        try:
            if not os.path.exists(self.imagenes_dir):
                os.makedirs(self.imagenes_dir, exist_ok=True)
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            unique_id = str(uuid.uuid4())[:8]
            filename = f"waste_{timestamp}_{unique_id}.jpg"
            filepath = os.path.join(self.imagenes_dir, filename)
            with open(filepath, 'wb') as f:
                f.write(imagen_bytes)
            return filepath
        except Exception as e:
            print(f"Servidor: Error al guardar imagen: {e}")
            return None