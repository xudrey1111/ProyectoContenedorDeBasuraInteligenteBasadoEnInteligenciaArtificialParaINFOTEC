#include <WiFi.h>              // Librería para la conexión WiFi
#include <HTTPClient.h>        // Librería para la comunicación HTTP
#include <ArduinoJson.h>       // Librería para manejar JSON
#include "esp_camera.h"        // Librería para la cámara del ESP32-CAM S3
#include "base64.h"            // Necesitarás una librería de Base64 (ej. `Base64.h` en Arduino Library Manager)

const char* ssid = "IZZI-FA5D";
const char* password = "QJY5MDZYWMLD";
const char* servidor = "http://192.168.0.121:5000/";
// ----------------------------------------------------------------------------
#define CAM_PIN_D0      11
#define CAM_PIN_D1      9
#define CAM_PIN_D2      8
#define CAM_PIN_D3      10
#define CAM_PIN_D4      12
#define CAM_PIN_D5      18
#define CAM_PIN_D6      17
#define CAM_PIN_D7      16

#define CAM_PIN_XCLK    15
#define CAM_PIN_PCLK    13
#define CAM_PIN_VSYNC   6

#define CAM_PIN_HREF    7
#define CAM_PIN_SDA    4
#define CAM_PIN_SCL    5

#define CAM_PIN_PWDN    -1
#define CAM_PIN_RESET   -1
// ----------------------------------------------------------------------------

String base64_encode(const unsigned char *data, size_t len);

// ----------------------------------------------------------------------------

/**
 * @brief Inicializa la cámara.
 */
bool init_camera() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;

    config.pin_d0 = CAM_PIN_D0;
    config.pin_d1 = CAM_PIN_D1;
    config.pin_d2 = CAM_PIN_D2;
    config.pin_d3 = CAM_PIN_D3;
    config.pin_d4 = CAM_PIN_D4;
    config.pin_d5 = CAM_PIN_D5;
    config.pin_d6 = CAM_PIN_D6;
    config.pin_d7 = CAM_PIN_D7;

    config.pin_xclk = CAM_PIN_XCLK;
    config.pin_pclk = CAM_PIN_PCLK;
    config.pin_vsync = CAM_PIN_VSYNC;

    config.pin_href = CAM_PIN_HREF;
    config.pin_sscb_sda = CAM_PIN_SDA;
    config.pin_sscb_scl = CAM_PIN_SCL;

    config.pin_pwdn = CAM_PIN_PWDN;
    config.pin_reset = CAM_PIN_RESET;

    config.xclk_freq_hz = 20000000;
    config.frame_size = FRAMESIZE_CIF;
    config.pixel_format = PIXFORMAT_JPEG;
    config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
    config.fb_location = CAMERA_FB_IN_DRAM;
    config.jpeg_quality = 30;
    config.fb_count = 1;
    
    int err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Es distinto a ok");
        return false;
    }
    return true;
}

/**
 * @brief Envía la detección del objeto por HTTP POST y procesa la respuesta.
 * * Envía un JSON indicando la detección y espera un JSON de respuesta
 * del servidor que contenga el campo "clasificacion" con el valor 'B' o 'N'.
 */
void enviarDeteccionYClasificacion() {
    if (WiFi.status() == WL_CONNECTED) {
        // 1. Capturar la imagen
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Fallo la captura de la camara");
            return;
        }
        // 2. Codificar la imagen a Base64
        // Usando Base64.h (requiere instalación)
        String image_base64 = base64::encode(fb->buf, fb->len);
        
        // Liberar el frame buffer de la cámara
        esp_camera_fb_return(fb);
        HTTPClient http;
        http.begin(servidor);
        
        http.addHeader("Content-Type", "application/json");
        DynamicJsonDocument doc(image_base64.length() + 512); // Aumentar tamaño para la imagen
        doc["evento"] = "objeto identificado";
        doc["dispositivo_id"] = WiFi.macAddress();
        doc["imagen_b64"] = image_base64; // Agregar la imagen en Base64
        
        String jsonPayload;
        serializeJson(doc, jsonPayload);
        Serial.print("Enviando POST de deteccion a ");
        Serial.println(servidor);
        // Serial.print("Payload: "); // NO Imprimir payload completo, es muy largo
        // Serial.println(jsonPayload);
        Serial.printf("Tamaño de imagen Base64: %d bytes\n", image_base64.length());
        int httpResponseCode = http.POST(jsonPayload);
        
        // 3. Procesar la respuesta
        if (httpResponseCode > 0) {
            Serial.printf("Código HTTP de respuesta: %d\n", httpResponseCode);
            String respuesta = http.getString();
            Serial.println("Respuesta del servidor:");
            Serial.println(respuesta);
            // 4. Parsear la respuesta para obtener la clasificación
            DynamicJsonDocument responseDoc(256);
            DeserializationError error = deserializeJson(responseDoc, respuesta);
            
            if (!error) {
                // Asume que el servidor responde con un campo llamado "clasificacion"
                const char* clasificacion = responseDoc["clasificacion"] | "Error";
                Serial.print("Clasificacion recibida: ");
                Serial.println(clasificacion);
                // 5. Procesar el comando de clasificación si es válido
                if (strlen(clasificacion) == 1 && (clasificacion[0] == 'B' || clasificacion[0] == 'N' || clasificacion[0] == 'b' || clasificacion[0] == 'n')) {
                    // procesarComando(clasificacion[0]); // Necesitas implementar esta función
                    Serial.printf("Comando procesado: %c\n", clasificacion[0]);
                } else {
                    Serial.println("Comando de clasificacion invalido o formato incorrecto.");
                }
            } else if (respuesta.length() > 0) {
                Serial.println("Error al interpretar JSON de respuesta, pero se recibio texto.");
            } else {
                Serial.println("No se recibio contenido en la respuesta.");
            }
        } else {
            Serial.printf("Error en la peticion HTTP: %s\n", http.errorToString(httpResponseCode).c_str());
        }
        http.end();
    } else {
        Serial.println("WiFi no conectado. Imposible enviar POST.");
    }
}
void setup() {
    Serial.begin(115200);
    Serial.println("\nIniciando ESP32...");
    // Inicializar la cámara
    if (!init_camera()) {
        Serial.println("Fallo la inicializacion de la camara. Reiniciando...");
        delay(5000);
        ESP.restart();
    }
    Serial.print("Conectando a WiFi a la red: ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi conectado exitosamente.");
        Serial.print("Direccion IP del ESP32: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\n¡Error de conexión WiFi! Revisa tus credenciales.");
    }
}

// Nota: Renombré la función en loop para que coincida con la modificada
void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        enviarDeteccionYClasificacion(); 
    }
    delay(10000); 

}

// Implementación simple de Base64 Encode (si no usas una librería)
// OJO: La librería externa Base64.h de Arduino es más robusta. 
// Es muy recomendable usar la librería externa 'Base64.h' de Adam Rudd
/*
String base64_encode(const unsigned char *data, size_t len) {
    // Implementación manual de Base64 si no usas la librería
    // Requiere la librería Base64.h
    return "Base64_Placeholder"; 
}
*/