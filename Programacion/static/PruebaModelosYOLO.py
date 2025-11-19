import os  
import cv2
import numpy as np
from ultralytics import YOLO  

class pruebaModeloIA:
    def __init__(self, path):
        self.modelo = self.loadModel(path)
        self.clases = {
            0: "biodegradable",
            1: "carton",
            2: "vidrio",
            3: "metal",
            4: "papel",
            5: "plastico"
        }

    def loadModel(self, ruta_modelo):
        if not os.path.exists(ruta_modelo):
            raise FileNotFoundError(f"No se encontró el archivo del modelo en {ruta_modelo}")
        modelo = YOLO(ruta_modelo)
        return modelo

    def processImage(self, imagen):
        results = self.modelo(imagen)
        return results  

    def predictImage(self, img_array):        
        for result in img_array:
            clases = result.boxes.cls.cpu().numpy()
            confidences = result.boxes.conf.cpu().numpy()
            indice_max = np.argmax(confidences)
            max_confianza = confidences[indice_max]
            clase_max = clases[indice_max]
            label = self.clases.get(int(clase_max), "Desconocido")
            return {
                'clase': label,
                'probabilidad': float(max_confianza),
            }

    def showResults(self, results):
        for result in results:
            img_resultado = result.plot()
            cv2.imshow("Resultado de la detección", img_resultado)
            cv2.waitKey(0)
            cv2.destroyAllWindows()

    def run(self, imagen):
        try:
            img_array = self.processImage(imagen)
            prediccion = self.predictImage(img_array)
            print(f"Predicción: {prediccion}")
            self.showResults(img_array)
        except Exception as e:
            print(f"\nError: {str(e)}")


if __name__ == "__main__":
    model_path = "C:\\Users\\XxGho\\OneDrive\\Documentos\\Escuela\\Proceso Dual\\Proyecto\\3° Proyecto\\Programacion\\static\\Modelos\\Identificacion de objetos\\yoloooo.pt"
    img_path = "C:\\Users\\XxGho\\OneDrive\\Documentos\\Escuela\\Proceso Dual\\Proyecto\\3° Proyecto\\Programacion\\static\\imagenes\\waste_20251116_195935_d98ac025.jpg"
    
    alpha = pruebaModeloIA(model_path)
    resultados = alpha.run(img_path)
