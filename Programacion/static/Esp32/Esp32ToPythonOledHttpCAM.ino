#include <Adafruit_GFX.h>       // Librería para gráficos
#include <Adafruit_SSD1306.h>   // Librería para el controlador de pantalla OLED
#include <Wire.h>               // Librería para la comunicación I2C

#include <NewPing.h>            // Librería para el sensor ultrasónico
#include <ESP32Servo.h>         // Librería para el control del servomotor en ESP32

#include <WiFi.h>               // Librería para la conexión WiFi
#include <HTTPClient.h>         // Librería para la comunicación HTTP
#include <ArduinoJson.h>        // Librería para manejar JSON
#include "esp_camera.h"        // Librería para la cámara del ESP32-CAM S3

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
 * @param tamano El tamaño de texto que se mostrará en la pantalla.
 * @param tiempo El tiempo en que el texto que se mostrará en la pantalla.
 */
void mostrarMensajeTemporal(String mensaje, int tamano = 1, int tiempo = 3000) {
    display.clearDisplay();     // Borra todo el contenido anterior
    display.setTextSize(tamano);     // Tamaño de letra doble (más grande y legible)
    display.setCursor(0, 0);    // Coloca el cursor en la esquina superior izquierda
    display.println(mensaje);   // Muestra el mensaje
    display.display();          // Actualiza la pantalla física para que el mensaje sea visible
    delay(tiempo);                // Espera 3 segundos
    display.clearDisplay();     // Borra la pantalla al terminar el tiempo
    display.display();          // Actualiza la pantalla para que quede en blanco
}

/**
 * @brief Mueve el servomotor de forma suave hacia un ángulo objetivo.
 * @param anguloObjetivo El ángulo al que se desea mover el servo.
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
 * @param anguloObjetivo El ángulo al que se desea mover el servo antes de regresar.
 */
void moverYVolverSuave(int anguloObjetivo) {
    moverServoSuave(anguloObjetivo);
    delay(500);         
    moverServoSuave(90);   
}

/**
 * @brief Procesa los comandos recibidos y mueve el servo.
 * @param cmd El comando de clasificación recibido ('B' o 'N').
 */
void procesarComando(char cmd) {
    for (int i = 0; i < 2; i++) {
        if (cmd == comandos[i] || cmd == tolower(comandos[i])) {
            moverYVolverSuave(angulos[i]);
            if (cmd == 'B' || cmd == 'b') {
                mostrarMensajeTemporal("Basura\nBiodegradable", 2, 3000);
            } else {
                mostrarMensajeTemporal("Basura\nNo-Biodegradable", 2, 3000);
            }
            return;
        }
    }
    mostrarMensajeTemporal("Comando de clasificacion invalido o no reconocido", 1, 3000);
    Serial.println("Comando de clasificacion invalido o no reconocido.");
}
/**
 * @brief Envía que maneja el envio del objeto identificado
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
 * @brief Función que maneja el envio de bytes de la imagen
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
            
            DynamicJsonDocument responseDoc(512);
            DeserializationError error = deserializeJson(responseDoc, respuesta);
            if (!error) {
                const char* status = responseDoc["status"] | "error";
                
                if (strcmp(status, "reintentar") == 0) {
                    const char* mensaje = responseDoc["message"] | "Confianza insuficiente";
                    float confianza = responseDoc["confianza_actual"] | 0.0;
                    
                    Serial.printf("Confianza insuficiente: %.2f%%, mensaje: %s\n", confianza * 100, mensaje);
                    mostrarMensajeTemporal("Confianza baja\nReintente", 2, 3000);
                    
                    delay(3000);
                    
                }
                else if (strcmp(status, "ok") == 0) {
                    const char* clasificacion = responseDoc["clasificacion"] | "Error";
                    if (strlen(clasificacion) == 1 && (clasificacion[0] == 'B' || clasificacion[0] == 'N')) {
                        Serial.printf("Comando procesado: %c\n", clasificacion[0]);
                        procesarComando(clasificacion[0]);
                    } else {
                        Serial.println("Comando de clasificacion invalido.");
                    }
                } else {
                    Serial.println("Error en la respuesta del servidor.");
                    mostrarMensajeTemporal("Error servidor", 1, 3000);
                }
            } else {
                Serial.println("Error parseando respuesta JSON");
                mostrarMensajeTemporal("Error JSON", 1, 3000);
            }
        } else {
            Serial.printf("Error enviando imagen: %s\n", http.errorToString(httpResponseCode).c_str());
            mostrarMensajeTemporal("Error envio", 1, 3000);
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
        delay(2000);
        enviarImagenComoBytes();
    }
}

// --- Configuración inicial (se ejecuta una vez) ---

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Inicializar I2C primero
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    
    // Inicializar pantalla OLED
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println(F("Error en la inicializacion de la pantalla OLED"));
    } else {
        Serial.println("Pantalla OLED inicializada");
        mostrarMensajeTemporal("Sistema iniciando", 1, 2000);
    }

    // Inicializar servo
    servoMotor.attach(SERVO_PIN);
    moverServoSuave(90);
    Serial.println("Servo inicializado");

    // Inicializar cámara
    if (!init_camera()) {
        Serial.println("Fallo la inicializacion de la camara");
        mostrarMensajeTemporal("Error Camara", 1, 3000);
    } else {
        mostrarMensajeTemporal("Camara inicializada correctamente", 1, 3000);
    }

    // Conexión a WiFi
    Serial.println("Conectando a WiFi...");
    mostrarMensajeTemporal("Conectando WiFi", 1, 2000);
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
        mostrarMensajeTemporal("Error WiFi", 1, 3000);
    }
    Serial.println("Sistema listo");
    mostrarMensajeTemporal("Sistema listo", 1, 2000);
}

// --- Bucle principal del programa ---

void loop() {
    // Variable estática para controlar el tiempo entre detecciones (debounce)
    static unsigned long ultima_deteccion = 0;
    unsigned long tiempo_actual = millis();
    
    // Obtiene la distancia en centímetros
    int distancia = sonar.ping_cm(); 
    
    // Lógica de detección: si se detecta un objeto (distancia válida y dentro del rango),
    // y ha pasado suficiente tiempo desde la última detección (3 segundos de debounce).
    if (distancia > 0 && distancia < MAX_DISTANCE && (tiempo_actual - ultima_deteccion) > 3000) {
        mostrarMensajeTemporal("Objeto identificado", 2, 2000);
        enviarDeteccionYClasificacion();
    }
    delay(50); // Pequeño delay para no saturar el loop
}