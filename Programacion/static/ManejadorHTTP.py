from flask import Flask, request, jsonify
import socket

class Manejador():
    def __init__(self):
        self.data=None

    def recepcionMensaje(self):
        if not request.is_json:
            print("Servidor: Error - La solicitud no contiene formato JSON.")
            return jsonify({"status": "error", "message": "Se espera JSON"}), 400

        self.data = request.get_json()
        return self.data
    
    def enviarMensaje(self):
        if self.data.get("evento") == "objeto identificado":
            print("Servidor: 'Objeto identificado' recibido. Iniciando clasificación (simulada)...")
            clasificacion = input("Dado que nos falta material, simule la clasificación ('B' o 'N'): ").upper()
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