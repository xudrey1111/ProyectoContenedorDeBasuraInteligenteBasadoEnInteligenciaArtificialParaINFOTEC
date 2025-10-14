#include <WiFi.h>              // Librería para la conexión WiFi
#include <HTTPClient.h>        // Librería para la comunicación HTTP
#include <ArduinoJson.h>       // Librería para manejar JSON
#include "esp_camera.h"        // Librería para la cámara del ESP32-CAM S3

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
 * @brief Envía primero la notificación de objeto identificado
 */
void enviarNotificacionDeteccion() {
        HTTPClient http;
        http.begin(servidor);
        http.setTimeout(10000);
        http.addHeader("Content-Type", "application/json");
        
        DynamicJsonDocument doc(256);
        doc["evento"] = "objeto identificado";
        doc["dispositivo_id"] = WiFi.macAddress();
        
        String jsonPayload;
        serializeJson(doc, jsonPayload);
    
        Serial.println("Enviando notificación de detección...");
        int httpResponseCode = http.POST(jsonPayload);
        if (httpResponseCode > 0) {
            Serial.printf("Notificación enviada. Código HTTP: %d\n", httpResponseCode);
        } else {
            Serial.printf("Error en notificación: %s\n", http.errorToString(httpResponseCode).c_str());
        }
        http.end();
}

/**
 * @brief Función principal que coordina el envío en dos pasos
 */
void enviarImagenComoBytes() {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Fallo la captura de la cámara");
            return;
        }
        HTTPClient http;
        http.begin(servidor);
        http.setTimeout(15000);
        http.addHeader("Content-Type", "application/json");
        
        DynamicJsonDocument doc(JSON_ARRAY_SIZE(fb->len) + 1024);
        doc["evento"] = "imagen bytes";
        
        JsonArray imagenArray = doc.createNestedArray("imagen_bytes");
        for (size_t i = 0; i < fb->len; i++) {
            imagenArray.add(fb->buf[i]);
        }
        doc["tamano_imagen"] = fb->len;
        
        String jsonPayload;
        serializeJson(doc, jsonPayload);
        
        Serial.print("Enviando imagen como bytes. Tamaño: ");
        Serial.print(fb->len);
        Serial.print(" bytes, Tamaño JSON: ");
        Serial.print(jsonPayload.length());
        Serial.println(" bytes");
        
        int httpResponseCode = http.POST(jsonPayload);
        
        if (httpResponseCode > 0) {
            Serial.printf("Imagen enviada. Código HTTP: %d\n", httpResponseCode);
            String respuesta = http.getString();
            Serial.println("Respuesta del servidor:");
            Serial.println(respuesta);
            
            DynamicJsonDocument responseDoc(256);
            DeserializationError error = deserializeJson(responseDoc, respuesta);
            if (!error) {
                const char* clasificacion = responseDoc["clasificacion"] | "Error";
                Serial.print("Clasificacion recibida: ");
                Serial.println(clasificacion);
                
                if (strlen(clasificacion) == 1 && (clasificacion[0] == 'B' || clasificacion[0] == 'N' || clasificacion[0] == 'b' || clasificacion[0] == 'n')) {
                    Serial.printf("Comando procesado: %c\n", clasificacion[0]);
                } else {
                    Serial.println("Comando de clasificacion invalido.");
                }
            }
        } else {
            Serial.printf("Error enviando imagen: %s\n", http.errorToString(httpResponseCode).c_str());
        }
        esp_camera_fb_return(fb);
        http.end();
}

void enviarDeteccionYClasificacion() {
    if (WiFi.status() == WL_CONNECTED) {
        enviarNotificacionDeteccion();
        Serial.println("Esperando 2 segundos antes de enviar imagen...");
        delay(2000);
        enviarImagenComoBytes();
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("\nIniciando ESP32...");
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

void loop() {
    if (Serial.available() > 0) {
        char comando = Serial.read();
        while (Serial.available() > 0) {
            Serial.read();
        }
        if (comando == '1') {
            Serial.println("Comando '1' recibido - Enviando detección...");
            enviarDeteccionYClasificacion();
        } else if (comando == '\n' || comando == '\r') {
            // Ignorar caracteres de nueva línea
        } else {
            Serial.print("Comando no reconocido: '");
            Serial.print(comando);
            Serial.println("'");
            Serial.println("Comandos disponibles: '1' - Enviar detección");
        }
    }
    delay(100);
}