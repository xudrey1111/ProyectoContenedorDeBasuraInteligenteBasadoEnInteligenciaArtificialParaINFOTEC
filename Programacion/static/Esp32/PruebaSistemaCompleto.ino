#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#include <NewPing.h>
#include <ESP32Servo.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "esp_camera.h"

// --- Definiciones de hardware y pines ---

// Dimensiones de la pantalla OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// Pines para la comunicación I2C con la pantalla OLED
#define I2C_SDA_PIN 14
#define I2C_SCL_PIN 21

// Pines para el sensor ultrasónico y el servomotor
#define TRIG_PIN 1
#define ECHO_PIN 2
#define SERVO_PIN 3
#define MAX_DISTANCE 10

// --- Configuración de pines de la cámara ESP32-S3 CAM ---
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
#define CAM_PIN_SDA     4
#define CAM_PIN_SCL     5

#define CAM_PIN_PWDN    -1
#define CAM_PIN_RESET   -1

// --- Credenciales de WiFi y servidor HTTP ---
const char* ssid = "IZZI-FA5D";
const char* password = "QJY5MDZYWMLD";
const char* servidor = "http://192.168.0.121:5000/";

// --- Creación de objetos ---
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);
Servo servoMotor;

// --- Variables de estado y constantes ---
const byte angulos[2] = {0, 180};
const char comandos[2] = {'B', 'N'};
const int velocidad = 10;

// --- Funciones del sistema ---

/**
 * @brief Inicializa la cámara con configuración simplificada.
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
 * @brief Muestra un mensaje temporal en la pantalla OLED por 3 segundos.
 */
void mostrarMensajeTemporal(String mensaje) {
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(mensaje);
    display.display();
    delay(3000);
    display.clearDisplay();
    display.display();
}

/**
 * @brief Mueve el servomotor de forma suave hacia un ángulo objetivo.
 */
void moverServoSuave(int anguloObjetivo) {
    int posicionActual = servoMotor.read();
    int incremento = (anguloObjetivo > posicionActual) ? 1 : -1;
    
    while (posicionActual != anguloObjetivo) {
        posicionActual += incremento;
        if ((incremento == 1 && posicionActual > anguloObjetivo) || 
            (incremento == -1 && posicionActual < anguloObjetivo)) {
            posicionActual = anguloObjetivo;
        }
        servoMotor.write(posicionActual); 
        delay(velocidad);                   
    }
}

/**
 * @brief Mueve el servo a un ángulo específico y luego lo regresa al centro.
 */
void moverYVolverSuave(int anguloObjetivo) {
    moverServoSuave(anguloObjetivo);
    delay(500);         
    moverServoSuave(90);   
}

/**
 * @brief Procesa los comandos recibidos y mueve el servo.
 */
void procesarComando(char cmd) {
    for (int i = 0; i < 2; i++) {
        if (cmd == comandos[i] || cmd == tolower(comandos[i])) {
            moverYVolverSuave(angulos[i]);
            if (cmd == 'B' || cmd == 'b') {
                mostrarMensajeTemporal("Basura\nBiodegradable");
            } else {
                mostrarMensajeTemporal("Basura\nNo-Biodegradable");
            }
            return;
        }
    }
    Serial.println("Comando de clasificacion invalido o no reconocido.");
}

/**
 * @brief Envía notificación de detección al servidor.
 */
void enviarNotificacionDeteccion() {
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi no conectado para enviar notificación");
        return;
    }
    
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
                    procesarComando(clasificacion[0])
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

/**
 * @brief Función principal que coordina el envío en dos pasos
 */
void enviarDeteccionYClasificacion() {
    if (WiFi.status() == WL_CONNECTED) {
        enviarNotificacionDeteccion();
        delay(1000);
        enviarImagenComoBytes();
    } else {
        Serial.println("WiFi no conectado, no se puede enviar detección");
    }
}

// --- Configuración inicial ---
void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("Iniciando sistema...");

    // Inicializar I2C primero
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    
    // Inicializar pantalla OLED
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("Error en la inicializacion de la pantalla OLED"));
    } else {
        Serial.println("Pantalla OLED inicializada");
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(0, 0);
        display.println("Sistema iniciando");
        display.display();
    }

    // Inicializar servo
    servoMotor.attach(SERVO_PIN);
    moverServoSuave(90);
    Serial.println("Servo inicializado");

    // Inicializar cámara
    if (!init_camera()) {
        Serial.println("Fallo la inicializacion de la camara");
        mostrarMensajeTemporal("Error Camara");
    } else {
        Serial.println("Camara inicializada correctamente");
    }

    // Conexión a WiFi
    Serial.println("Conectando a WiFi...");
    mostrarMensajeTemporal("Conectando WiFi");
    WiFi.begin(ssid, password);
    
    int intentos = 0;
    while (WiFi.status() != WL_CONNECTED && intentos < 20) {
        delay(500);
        Serial.print(".");
        intentos++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi conectado");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        mostrarMensajeTemporal("WiFi OK\nIP: " + WiFi.localIP().toString());
    } else {
        Serial.println("\nError de conexión WiFi!");
        mostrarMensajeTemporal("Error WiFi");
    }
    
    display.clearDisplay();
    display.display();
    Serial.println("Sistema listo");
}

// --- Bucle principal ---
void loop() {
    static unsigned long ultima_deteccion = 0;
    unsigned long tiempo_actual = millis();
    
    // Leer sensor de distancia
    int distancia = sonar.ping_cm();
    
    // Procesar comandos seriales
    if (Serial.available() > 0) {
        char comando = Serial.read();
        // Limpiar buffer serial
        while (Serial.available() > 0) {
            Serial.read();
        }
        
        if (comando == '1') {
            if ((tiempo_actual - ultima_deteccion) > 3000) {
                ultima_deteccion = tiempo_actual;
                Serial.println("Comando de detección recibido");
                mostrarMensajeTemporal("Objeto detectado");
                enviarDeteccionYClasificacion();
            } else {
                Serial.println("Espera entre detecciones");
            }
        } else if (comando == '\n' || comando == '\r') {
            // Ignorar caracteres de nueva línea
        } else {
            Serial.print("Comando no reconocido: '");
            Serial.print(comando);
            Serial.println("'");
            Serial.println("Comandos: '1' - Enviar detección");
        }
    }
    
    delay(100);
}
