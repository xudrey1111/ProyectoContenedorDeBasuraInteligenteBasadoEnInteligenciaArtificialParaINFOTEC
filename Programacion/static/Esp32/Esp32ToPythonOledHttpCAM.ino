#include <Adafruit_GFX.h>       // Librería para gráficos
#include <Adafruit_SSD1306.h>   // Librería para el controlador de pantalla OLED
#include <Wire.h>               // Librería para la comunicación I2C
#include <NewPing.h>            // Librería para el sensor ultrasónico
#include <ESP32Servo.h>         // Librería para el control del servomotor en ESP32
#include <WiFi.h>               // Librería para la conexión WiFi
#include <HTTPClient.h>         // Librería para la comunicación HTTP
#include <ArduinoJson.h>        // Librería para manejar JSON
#include "esp_camera.h"        // Librería para la cámara del ESP32-CAM S3
#include "base64.h"            // Necesitarás una librería de Base64

// --- Definiciones de hardware y pines ---

// Dimensiones de la pantalla OLED
#define SCREEN_WIDTH 128        // Ancho de la pantalla en píxeles
#define SCREEN_HEIGHT 64        // Alto de la pantalla en píxeles
#define OLED_RESET -1           // Pin de reset

// Pines para la comunicación I2C con la pantalla OLED
#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9

// Pines para el sensor ultrasónico y el servomotor
#define TRIG_PIN 1              // Pin del Trigger del sensor ultrasónico
#define ECHO_PIN 3              // Pin del Echo del sensor ultrasónico
#define SERVO_PIN 12            // Pin de control para el servomotor
#define MAX_DISTANCE 10         // Distancia máxima en cm para detectar un objeto

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

String base64_encode(const unsigned char *data, size_t len);


// --- Credenciales de WiFi y servidor HTTP ---

const char* ssid = "IZZI-FA5D";
const char* password = "QJY5MDZYWMLD";
const char* servidor = "http://192.168.0.121:5000/"; 

// --- Creación de objetos ---

// Objeto para controlar la pantalla OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
// Objeto para el sensor ultrasónico, con sus pines y distancia máxima
NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);
// Objeto para controlar el servomotor
Servo servoMotor;

// --- Variables de estado y constantes ---

// Ángulos de los movimientos del servo para cada tipo de basura (ej. 0° y 180°)
const byte angulos[2] = {0, 180};
// Comandos esperados para clasificar ('B' para Biodegradable, 'N' para No-biodegradable)
const char comandos[2] = {'B', 'N'};
// Velocidad del movimiento del servo (menor valor = más rápido)
const int velocidad = 10;

// --- Funciones del sistema ---

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
 * @brief Muestra un mensaje temporal en la pantalla OLED por 3 segundos.
 * @param mensaje El texto que se mostrará en la pantalla.
 */
void mostrarMensajeTemporal(String mensaje) {
    display.clearDisplay();     // Borra todo el contenido anterior
    display.setCursor(0, 0);    // Coloca el cursor en la esquina superior izquierda
    display.println(mensaje);   // Muestra el mensaje
    display.display();          // Actualiza la pantalla física para que el mensaje sea visible
    delay(3000);                // Espera 3 segundos
    display.clearDisplay();     // Borra la pantalla al terminar el tiempo
    display.display();          // Actualiza la pantalla para que quede en blanco
}

/**
 * @brief Mueve el servomotor de forma suave hacia un ángulo objetivo.
 * @param anguloObjetivo El ángulo al que se desea mover el servo.
 */
void moverServoSuave(int anguloObjetivo) {
    int posicionActual = servoMotor.read();
    // Determina la dirección del movimiento (incremento o decremento)
    int incremento = (anguloObjetivo > posicionActual) ? 1 : -1;
    
    // Bucle para mover el servo paso a paso
    while (posicionActual != anguloObjetivo) {
        posicionActual += incremento;
        // Asegura que el servo no se pase del ángulo objetivo
        if ((incremento == 1 && posicionActual > anguloObjetivo) || 
            (incremento == -1 && posicionActual < anguloObjetivo)) {
            posicionActual = anguloObjetivo;
        }
        servoMotor.write(posicionActual); // Envía el comando de movimiento
        delay(velocidad);                   // Pausa breve para lograr un movimiento suave
    }
}

/**
 * @brief Mueve el servo a un ángulo específico y luego lo regresa al centro.
 * @param anguloObjetivo El ángulo al que se desea mover el servo antes de regresar.
 */
void moverYVolverSuave(int anguloObjetivo) {
    moverServoSuave(anguloObjetivo);
    delay(500);         // Espera medio segundo
    moverServoSuave(90);    // Regresa el servo a la posición central (90°)
}

/**
 * @brief Procesa los comandos recibidos y mueve el servo.
 * @param cmd El comando de clasificación recibido ('B' o 'N').
 */
void procesarComando(char cmd) {
    for (int i = 0; i < 2; i++) {
        // Compara el comando recibido con los comandos definidos ('B' o 'N')
        if (cmd == comandos[i] || cmd == tolower(comandos[i])) {
            Serial.println("Comando procesado, moviendo servo.");
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
 * @brief Envía la detección del objeto por HTTP POST y procesa la respuesta.
 * * Envía un JSON indicando la detección y espera un JSON de respuesta
 * del servidor que contenga el campo "clasificacion" con el valor 'B' o 'N'.
 */
void enviarDeteccionYClasificacion() {
    if (WiFi.status() == WL_CONNECTED) {
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            Serial.println("Fallo la captura de la camara");
            return;
        }
        String image_base64 = base64::encode(fb->buf, fb->len);

        esp_camera_fb_return(fb);
        HTTPClient http;
        http.begin(servidor);
        
        // .Definir el encabezado Content-Type para enviar JSON
        http.addHeader("Content-Type", "application/json");
        DynamicJsonDocument doc(image_base64.length() + 512); // Aumentar tamaño para la imagen
        doc["evento"] = "objeto identificado";
        doc["dispositivo_id"] = WiFi.macAddress();
        doc["imagen_b64"] = image_base64; // Agregar la imagen en Base64
        
        String jsonPayload;
        serializeJson(doc, jsonPayload);
        Serial.print("Enviando POST de deteccion a ");
        Serial.println(servidor);
        Serial.printf("Tamaño de imagen Base64: %d bytes\n", image_base64.length());
        int httpResponseCode = http.POST(jsonPayload);
        
        //  Procesar la respuesta
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
                    procesarComando(clasificacion[0]);
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

// --- Configuración inicial (se ejecuta una vez) ---

void setup() {
    Serial.begin(115200);

    // Inicialización de cámara
    if (!init_camera()) {
        Serial.println("Fallo la inicializacion de la camara. Reiniciando...");
        delay(5000);
        ESP.restart();
    }

    // Inicialización de componentes físicos
    servoMotor.attach(SERVO_PIN); // Asocia el objeto servo al pin físico
    moverServoSuave(90); // Mueve el servo a la posición central inicial de forma suave
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN); // Inicia la comunicación I2C
    
    // Inicialización de la pantalla OLED
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Dirección I2C: 0x3C
        Serial.println(F("Error en la inicializacion de la pantalla OLED"));
        for(;;); // Detiene el programa en caso de error
    }

    // Conexión a WiFi
    Serial.println("Conectando a WiFi...");
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Conectando...");
    display.display();
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi conectado.");
        Serial.print("Direccion IP: ");
        Serial.println(WiFi.localIP());
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("WiFi Conectado!");
        display.println(WiFi.localIP());
        display.display();
        delay(2000);
    } else {
        Serial.println("\n¡Error de conexión WiFi!");
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("ERROR");
        display.println("WiFi");
        display.display();
    }
    
    display.clearDisplay();
    display.display();
}

// --- Bucle principal del programa ---

void loop() {
    // Variable estática para controlar el tiempo entre detecciones (debounce)
    static unsigned long ultima_deteccion = 0;
    unsigned long tiempo_actual = millis();
    
    // Obtiene la distancia en centímetros
    int distancia = sonar.ping_cm(); 
    
    // Lógica de detección: si hay conexión WiFi, se detecta un objeto (distancia válida y dentro del rango),
    // y ha pasado suficiente tiempo desde la última detección (3 segundos de debounce).
    if (WiFi.status() == WL_CONNECTED && distancia > 0 && distancia < MAX_DISTANCE && (tiempo_actual - ultima_deteccion) > 3000) {
        ultima_deteccion = tiempo_actual;
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Objeto");
        display.println("Detectado!");
        display.display();
        
        // Ejecuta la detección, envío POST, espera de respuesta, y procesamiento de clasificación
        enviarDeteccionYClasificacion();
        
        // El POST es bloqueante, por lo que el programa se detiene aquí hasta recibir la respuesta 
        // o hasta que la conexión falle. El debounce se encarga de evitar ejecuciones rápidas sucesivas.
    }
    
    delay(50); // Pequeño delay para no saturar el loop
}