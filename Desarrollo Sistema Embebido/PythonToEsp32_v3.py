from conexioEsp32WS import *
#from pruebaModelos import *

class PythonToESP32:
    def __init__(self,host, port,imagen,modelo):
        self.alpha= WebSocketConnection(host, port, self.comunicacion)
        #self.bravo= pruebaModeloIA(imagen,modelo)

    async def comunicacion(self,websocket):
        print(f"Nuevo cliente conectado desde {websocket.remote_address}")
        try:
            while True:
                message = await self.alpha.reciboMensajes(websocket)
                print(f"← Recibido del cliente: {message}")
                if message == "objeto identificado":
                    print("Servidor: 'Iniciando detección de IA...'")
                    response = input("Dado que nos falta material para lograr eso, se simulará la respuesta de la IA: ")
                    await self.alpha.envioMensajes(websocket,response) 
                    print(f"→ Enviando al cliente: {response}")
                    received_ack = await self.alpha.reciboMensajes(websocket)
                    print(f"← Recibido del cliente: {received_ack}")
                    if received_ack == "recibido":
                        print("Servidor: Confirmación 'recibido' por parte del cliente.")                        
                    else:
                        print("Servidor: Se esperaba el mensaje 'recibido' pero se recibió otro.")
                else:
                    print(f"Servidor: Se esperaba 'objeto identificado' pero se recibió '{message}'.")
        except ConnectionClosedError:
            print(f"Conexión con el cliente {websocket.remote_address} cerrada.")
        except Exception as e:
            print(f"Ocurrió un error en la conexión: {e}")

    async def ejecuctar(self):
        await self.alpha.iniciarServidor()
