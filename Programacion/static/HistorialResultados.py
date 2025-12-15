from collections import OrderedDict
import time
from flask import jsonify, request

class estadisticasIdentificaciones():
    def __init__(self):
        self.generalIdentificacion = OrderedDict()
        self.contador = 0
        self.estadisticasClases = {}
    
    def setDiccionarioPrincipal(self,dict):
        self.contador += 1
        entrada_historial = {
            'id': f"id_{self.contador:04d}",
            'timestamp': time.time(),
            'clase': dict.get('clase', 'Desconocido'),
            'probabilidad': dict.get('probabilidad', 0.0),
            'imagen_path': dict.get('imagen_path', ''),
            'fecha_hora': time.strftime('%Y-%m-%d %H:%M:%S')
        }
        self.generalIdentificacion[f"id_{self.contador:04d}"] = entrada_historial
        return self.generalIdentificacion
    
    def getHistorial(self):
        return jsonify({
            'total_identificaciones': self.contador,
            'historial': list(self.generalIdentificacion.values())
        })
    
    def getEstadisticas(self):
        clases = [item['clase'] for item in self.generalIdentificacion.values()]
        for clase in clases:
            self.estadisticasClases[clase] = self.estadisticasClases.get(clase, 0) + 1
            promedio_confianza = sum(item['probabilidad'] for item in self.generalIdentificacion.values()) / len(self.generalIdentificacion)
        return jsonify({
            'total_identificaciones': self.contador,
            'total_en_historial': len(self.generalIdentificacion),
            'promedio_confianza': round(promedio_confianza, 4),
            'conteo_por_clase': self.estadisticasClases
        })