#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "IZZI-FA5D";
const char* password = "QJY5MDZYWMLD";
const char* servidor = "http://192.168.0.121:5000/upload"; // URL del servidor

void setup() {
    Serial.begin(115200);

    Serial.print("Intentando conectar a WiFi...");
    WiFi.begin(ssid, password);
    int max_attempts = 20;
    while (WiFi.status() != WL_CONNECTED && max_attempts > 0) {
        delay(500);
        Serial.print(".");
        max_attempts--;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi conectado exitosamente!");
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\n¡Error de conexión WiFi!");
    }
}

// ======= FUNCIÓN PARA ENVIAR MENSAJE POST =======
void enviarMensajePost() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        http.begin(servidor);
    
        // 1. Definir el encabezado Content-Type para enviar JSON
        http.addHeader("Content-Type", "application/json");

        // 2. Crear el payload/mensaje en formato JSON
        DynamicJsonDocument doc(256);
        doc["estado"] = "prueba_ok";
        doc["mensaje"] = "objeto identificado";
        doc["dispositivo_id"] = WiFi.macAddress();
    
        String jsonPayload;
        serializeJson(doc, jsonPayload);

        Serial.print("Enviando POST a ");
        Serial.println(servidor);
        Serial.print("Payload: ");
        Serial.println(jsonPayload);

        // 3. Realizar la petición POST con el mensaje JSON
        int httpResponseCode = http.POST(jsonPayload);

        // 4. Procesar la respuesta
        if (httpResponseCode > 0) {
            Serial.printf("Código HTTP de respuesta: %d\n", httpResponseCode);
            String respuesta = http.getString();
            Serial.println("Respuesta del servidor:");
            Serial.println(respuesta);
            // Opcional: Intenta parsear la respuesta si esperas JSON de vuelta
            DynamicJsonDocument responseDoc(256);
            DeserializationError error = deserializeJson(responseDoc, respuesta);
            if (!error) {
                // Ejemplo de lectura de un campo de respuesta
                const char* status = responseDoc["status"] | "No status provided";
                Serial.print("Estado del servidor (en JSON): ");
                Serial.println(status);
            } else if (respuesta.length() > 0) {
                Serial.println("Error al interpretar JSON de respuesta, pero se recibió texto.");
            } else {
                Serial.println("No se recibió contenido en la respuesta.");
            }
    } 
        else {
            Serial.printf("Error en la petición HTTP: %s\n", http.errorToString(httpResponseCode).c_str());
    }
        http.end();
    } 
    else {
        Serial.println("WiFi no conectado. Revisar la conexión antes de intentar POST.");
    }
}

// ======= LOOP PRINCIPAL =======
void loop() {
    enviarMensajePost();
    // Espera 5 segundos antes de enviar la siguiente prueba
    delay(5000); 
}