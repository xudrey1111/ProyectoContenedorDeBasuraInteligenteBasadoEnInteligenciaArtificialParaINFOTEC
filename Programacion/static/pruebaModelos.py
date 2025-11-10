from tensorflow.keras.models import load_model
from tensorflow.keras.preprocessing import image
from tensorflow.keras.applications.mobilenet_v2 import preprocess_input 
import numpy as np
import matplotlib.pyplot as plt
import os

class pruebaModeloIA:
    """
        Inicializa la clase con la ruta del modelo y define las clases de clasificación
        Args:
            modelo (str): Ruta al archivo del modelo entrenado
    """
    def __init__(self, modelo):
        self.modelo = self.loadModel(modelo)
        self.clases = {0: 'Biodegradable', 1: 'No biodegradable'}

    """
        Carga el modelo de TensorFlow desde la ruta especificada
        Args:
            ruta_modelo (str): Ruta al archivo del modelo
        Returns:
            model: Modelo de TensorFlow cargado
        Raises:
            FileNotFoundError: Si el archivo del modelo no existe
    """
    def loadModel(self, ruta_modelo):
        if not os.path.exists(ruta_modelo):
            raise FileNotFoundError(f"No se encontró el archivo del modelo en {ruta_modelo}")
        modelo = load_model(ruta_modelo)
        return modelo

    """
        Preprocesa una imagen para que sea compatible con el modelo
        Args:
            imagen (str): Ruta a la imagen a procesar
        Returns:
            numpy.ndarray: Array de la imagen preprocesada con shape (1, 224, 224, 3)
    """
    def processImage(self,imagen):
        img = image.load_img(imagen, target_size=(224, 224))
        img_array = image.img_to_array(img)
        img_array = preprocess_input(img_array)
        img_array = np.expand_dims(img_array, axis=0)
        return img_array

    """
        Realiza la predicción sobre la imagen preprocesada
        Args:
            img_array (numpy.ndarray): Array de la imagen preprocesada
        Returns:
            dict: Diccionario con la clase predicha y su probabilidad
    """
    def predictImage(self, img_array):
        predicciones = self.modelo.predict(img_array)
        clase_predicha = np.argmax(predicciones[0])
        probabilidad = np.max(predicciones[0])
        nombre_clase = self.clases[clase_predicha]
        return {
            'clase': nombre_clase,
            'probabilidad': float(probabilidad)
        }

    """
        Muestra la imagen con la predicción usando matplotlib
        Args:
            imagen (str): Ruta a la imagen original
            prediccion (dict): Diccionario con los resultados de la predicción
        Returns:
            None: Muestra la imagen con la predicción
    """
    def showResults(self, imagen, prediccion):
        img = image.load_img(imagen)
        plt.figure(figsize=(8, 6))
        plt.imshow(img)
        plt.title(f"Predicción: {prediccion['clase']} ({prediccion['probabilidad']*100:.2f}%)")
        plt.axis('off')
        plt.show()

    """
        Ejecuta el pipeline completo: procesar imagen, predecir y mostrar resultados
        Args:
            imagen (str): Ruta a la imagen a clasificar
        Returns:
            None: Ejecuta todo el proceso de clasificación
    """
    def run(self,imagen):
        try:
            img_array = self.processImage(imagen)
            prediccion = self.predictImage(img_array)
            self.showResults(imagen,prediccion)
        except Exception as e:
            print(f"\nError: {str(e)}")