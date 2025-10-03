import asyncio
import websockets
from websockets.exceptions import ConnectionClosedError, InvalidURI, WebSocketException

class WebSocketConnection:
    def __init__(self, host: str, port: int, comunicacion):
        self.host = host
        self.port = port
        self.server = None
        self.comunicacion= comunicacion

    async def iniciarServidor(self):
        try:
            print(f"Iniciando servidor en ws://{self.host}:{self.port}...")
            self.server = await websockets.serve(self.comunicacion, self.host, self.port)
            print("Servidor listo y esperando conexiones.")
            await self.server.wait_closed()
            return True
        except (ConnectionRefusedError, InvalidURI) as e:
            print(f"Error de conexión: {e}")
            return False
        except WebSocketException as e:
            print(f"Error en WebSocket: {e}")
            return False

    async def reciboMensajes(self, websocket):
        return await websocket.recv()
    
    async def envioMensajes(self,websocket,mensaje):
        return await websocket.send(mensaje)

    async def cerrarServidor(self):
        if self.server:
            self.server.close()
            await self.server.wait_closed()
            print("Servidor WebSocket cerrado.")