#include <Adafruit_GFX.h>       // Librería para gráficos
#include <Adafruit_SSD1306.h>   // Librería para el controlador de pantalla OLED
#include <Wire.h>               // Librería para la comunicación I2C
#include <NewPing.h>            // Librería para el sensor ultrasónico
#include <ESP32Servo.h>         // Librería para el control del servomotor en ESP32
#include <WiFi.h>               // Librería para la conexión WiFi
#include <HTTPClient.h>         // Librería para la comunicación HTTP
#include <ArduinoJson.h>        // Librería para manejar JSON

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
            
            // Muestra el resultado en la pantalla OLED
            if (cmd == 'B' || cmd == 'b') {
                mostrarMensajeTemporal("Basura\nBiodegradable");
            } else {
                mostrarMensajeTemporal("Basura\nNo-Biodegradable");
            }
            // NOTA: El paso de enviar confirmación 'recibido' se omite 
            // ya que la clasificación se recibe en la respuesta síncrona del POST.
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
        HTTPClient http;
        http.begin(servidor);
        
        // 1. Definir el encabezado Content-Type para enviar JSON
        http.addHeader("Content-Type", "application/json");

        // 2. Crear el payload/mensaje en formato JSON para la detección
        DynamicJsonDocument doc(256);
        doc["evento"] = "objeto identificado";
        doc["dispositivo_id"] = WiFi.macAddress();
        
        String jsonPayload;
        serializeJson(doc, jsonPayload);

        Serial.print("Enviando POST de deteccion a ");
        Serial.println(servidor);
        Serial.print("Payload: ");
        Serial.println(jsonPayload);

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
    Serial.begin(9600);

    // Inicialización de componentes físicos
    servoMotor.attach(SERVO_PIN); // Asocia el objeto servo al pin físico
    moverServoSuave(90); // Mueve el servo a la posición central inicial de forma suave
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN); // Inicia la comunicación I2C
    
    // Inicialización de la pantalla OLED
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Dirección I2C: 0x3C
        Serial.println(F("Error en la inicializacion de la pantalla OLED"));
        for(;;); // Detiene el programa en caso de error
    }
    display.clearDisplay();
    display.setTextSize(2);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println("Sistema de");
    display.println("Clasificacion");
    display.println("Iniciado");
    display.display();
    delay(3000);
    display.clearDisplay();
    display.display();

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