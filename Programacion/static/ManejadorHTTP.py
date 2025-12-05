from datetime import datetime
import uuid
from flask import Flask, request, jsonify
from static.pruebaModelosTF import *
#from static.PruebaModelosYOLO import *
import socket

class Manejador():
    """
        Inicializa el manejador con el modelo de IA y configura las rutas
        Args:
            modelo (str): Ruta al modelo de IA entrenado
    """
    def __init__(self, modelo):
        self.data=None
        self.imagen=None
        self.diccionarioIdentificacion=None
        self.ultimaImagenCapturada = None
        self.no_biodegradables = ["No biodegradable", "plastico", "metal", "vidrio"]
        self.bravo=pruebaModeloIA(modelo)
        self.imagenes_dir = os.path.join('Programacion','static', 'imagenes')

    """
        Recibe y valida un mensaje JSON desde una solicitud HTTP
        Returns:
            tuple: (JSON data, status code) o (error response, 400) si no es JSON válido
    """
    def recepcionMensaje(self):
        if not request.is_json:
            print("Servidor: Error - La solicitud no contiene formato JSON.")
            return jsonify({"status": "error", "message": "Se espera JSON"}), 400

        self.data = request.get_json()
        return self.data
    
    """
        Procesa el mensaje recibido según el tipo de evento y ejecuta la lógica correspondiente
        Returns:
            tuple: (JSON response, status code) con los resultados del procesamiento
    """
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
                return self.logicaRecepcionImagen(imagen_bytes_array)
            except Exception as e:
                print(f"Servidor: Error al procesar la imagen: {str(e)}")
                self.diccionarioIdentificacion = {
                    'clase': 'Esperando detección...',
                    'probabilidad': 0.0,
                    'imagen_path': 'Imagenes/placeholder.jpg'
                }
                return jsonify({"status": "error", "message": f"Error al procesar la imagen: {str(e)}"}), 500
        else:
            return jsonify({"status": "error", "message": "Evento no reconocido"}), 400


    def logicaRecepcionImagen(self,imagen_bytes_array):
            imagen_bytes = bytes(imagen_bytes_array)
            print(f"Servidor: Imagen recibida como bytes - tamaño: {len(imagen_bytes)} bytes")
            self.imagen = self.guardarImagen(imagen_bytes)
            self.ultimaImagenCapturada = self.imagen

            procesarImagen = self.bravo.processImage(self.imagen)
            nombre_archivo = os.path.basename(self.imagen)
            ruta_web = f"imagenes/{nombre_archivo}" 

            self.diccionarioIdentificacion = self.bravo.predictImage(procesarImagen)
            getProbabilidadClase = self.diccionarioIdentificacion.get('probabilidad', 0)
            getNombreClase = self.diccionarioIdentificacion.get('clase', '')

            if getProbabilidadClase <= 0.60:
                print(f"Servidor: Confianza insuficiente ({getProbabilidadClase*100:.2f}%), solicitando reintento")
                response_data = {
                    "status": "reintentar", 
                    "message": "Confianza insuficiente, por favor reintente",
                    "confianza_actual": float(getProbabilidadClase)
                }
                self.diccionarioIdentificacion = {
                    'clase': "Objeto no identificado. Reintentar",
                    'probabilidad': float(getProbabilidadClase),
                }
                self.diccionarioIdentificacion['imagen_path'] = ruta_web
                return jsonify(response_data), 200
            else:
                self.diccionarioIdentificacion['imagen_path'] = ruta_web
                clasificacion = "N" if getNombreClase in self.no_biodegradables else "B"
                response_data = {
                    "status": "ok", 
                    "clasificacion": clasificacion,
                    "clase_detectada": getNombreClase,
                    "confianza": float(getProbabilidadClase)
                }
                return jsonify(response_data), 200

    """
        Obtiene la dirección IP del servidor
        Returns:
            str: Dirección IP del servidor o "0.0.0.0" si hay error
    """
    def getIpServidor(self):
        try:
            s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            s.connect(("8.8.8.8", 80))
            ip = s.getsockname()[0]
            s.close()
            return ip
        except Exception:
            return "0.0.0.0"

    """
        Guarda los bytes de la imagen en el sistema de archivos con nombre único
        Args:
            imagen_bytes (bytes): Bytes de la imagen a guardar
        Returns:
            str or None: Ruta del archivo guardado o None si hay error
    """
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