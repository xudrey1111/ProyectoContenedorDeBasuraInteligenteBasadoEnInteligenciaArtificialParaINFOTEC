from tensorflow.keras.models import load_model
from tensorflow.keras.preprocessing import image
from tensorflow.keras.applications.mobilenet_v2 import preprocess_input 
import numpy as np
import matplotlib.pyplot as plt
import os

class pruebaModeloIA:
    def __init__(self, imagen, modelo):
        self.modelo = self.loadModel(modelo)
        self.imagen = imagen
        self.clases = {0: 'Biodegradable', 1: 'No biodegradable'}

    def loadModel(self, ruta_modelo):
        if not os.path.exists(ruta_modelo):
            raise FileNotFoundError(f"No se encontró el archivo del modelo en {ruta_modelo}")
        modelo = load_model(ruta_modelo)
        return modelo

    def processImage(self,imagen, target_size=(224, 224)):
        img = image.load_img(imagen, target_size=target_size)
        img_array = image.img_to_array(img)
        img_array = preprocess_input(img_array)
        img_array = np.expand_dims(img_array, axis=0)
        return img_array

    def predictImage(self, img_array):
        predicciones = self.modelo.predict(img_array)
        clase_predicha = np.argmax(predicciones[0])
        probabilidad = np.max(predicciones[0])
        nombre_clase = self.clases[clase_predicha]
        
        return {
            'clase': nombre_clase,
            'probabilidad': float(probabilidad),
            'todas_las_probabilidades': predicciones[0].tolist()
        }

    def showResults(self, ruta_imagen, prediccion):
        img = image.load_img(ruta_imagen)
        plt.figure(figsize=(8, 6))
        plt.imshow(img)
        plt.title(f"Predicción: {prediccion['clase']} ({prediccion['probabilidad']*100:.2f}%)")
        plt.axis('off')
        print("\nProbabilidades por clase:")
        for idx, prob in enumerate(prediccion['todas_las_probabilidades']):
            print(f"{self.clases[idx]}: {prob*100:.2f}%")
        plt.show()

    def run(self):
        try:
            img_array = self.processImage()
            prediccion = self.predictImage(img_array)
            self.showResults(self.imagen, prediccion)
        except Exception as e:
            print(f"\nError: {str(e)}")