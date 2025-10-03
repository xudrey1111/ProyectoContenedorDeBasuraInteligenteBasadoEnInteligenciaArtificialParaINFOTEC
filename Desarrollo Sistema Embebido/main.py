from PythonToEsp32_v3 import *

if __name__ == "__main__":
    host_servidor = "192.168.0.121"
    puerto_servidor = 8765
    modelo="C:\\Users\\XxGho\\OneDrive\\Documentos\\Escuela\\Proceso Dual\\Proyecto\\2° Proyecto\\Python\\Modelos\\Identificacion de images\\predictWaste_mobilenetv2.h5"
    imagen=""
    charlie=PythonToESP32(host_servidor,puerto_servidor,imagen,modelo)
    try:
        asyncio.run(charlie.ejecuctar())
    except KeyboardInterrupt:
        print("Servidor detenido por el usuario.")