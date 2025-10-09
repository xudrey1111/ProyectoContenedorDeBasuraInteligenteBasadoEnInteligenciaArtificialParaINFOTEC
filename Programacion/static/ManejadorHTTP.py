from flask import Flask, request, jsonify
from static.pruebaModelos import *
import socket

class Manejador():
    def __init__(self, imagen, modelo):
        self.data=None
        self.imagen=imagen
        self.modelo=modelo
        self.bravo=pruebaModeloIA(imagen,modelo)

    def recepcionMensaje(self):
        if not request.is_json:
            print("Servidor: Error - La solicitud no contiene formato JSON.")
            return jsonify({"status": "error", "message": "Se espera JSON"}), 400

        self.data = request.get_json()
        return self.data
    
    def enviarMensaje(self):
        if self.data.get("evento") == "objeto identificado":
            print("Servidor: 'Objeto identificado' recibido. Iniciando clasificación (simulada)...")
            #reconstruirImagen="" aqui se reconstruye la imagen de los bytes enviados por esp32
            #self.imagen=reconstruirImagen Aquí se recarga la imagen a la clase
            procesarImagen = self.bravo.processImage(self.imagen)
            getNombreClase = self.bravo.predictImage(procesarImagen).get('clase', '')
            clasificacion = "N" if getNombreClase == "No Biodegradable" else "B"
            response_data = {"status": "ok", "clasificacion": clasificacion}
            print(f"→ Enviando al ESP32 (Respuesta HTTP 200): {response_data}")
            return jsonify(response_data), 200
        
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