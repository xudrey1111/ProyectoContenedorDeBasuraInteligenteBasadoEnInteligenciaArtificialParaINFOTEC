#include <Adafruit_GFX.h>       // Librería para gráficos, necesaria para la pantalla OLED
#include <Adafruit_SSD1306.h>   // Librería para el controlador de pantalla SSD1306 (OLED)
#include <Wire.h>               // Librería para la comunicación I2C (necesaria para el OLED)
#include <NewPing.h>            // Librería para el sensor ultrasónico
#include <ESP32Servo.h>         // Librería para el control del servomotor en ESP32
#include <WiFi.h>               // Librería para la conexión WiFi
#include <WebSocketsClient.h>   // Librería para el cliente WebSocket

// --- Definiciones de hardware y pines ---

// Dimensiones de la pantalla OLED
#define SCREEN_WIDTH 128    // Ancho de la pantalla en píxeles
#define SCREEN_HEIGHT 64    // Alto de la pantalla en píxeles
#define OLED_RESET -1       // Pin de reset (se usa -1 para ESP32, ya que no es un pin físico)

// Pines para la comunicación I2C con la pantalla OLED
#define I2C_SDA_PIN 8
#define I2C_SCL_PIN 9

// Pines para el sensor ultrasónico y el servomotor
#define TRIG_PIN 1          // Pin del Trigger del sensor ultrasónico
#define ECHO_PIN 3          // Pin del Echo del sensor ultrasónico
#define SERVO_PIN 12        // Pin de control para el servomotor
#define MAX_DISTANCE 10     // Distancia máxima en cm para detectar un objeto

// --- Credenciales de WiFi y servidor WebSocket ---

const char* ssid = "IZZI-FA5D";
const char* password = "QJY5MDZYWMLD";
const char* websocket_server_host = "192.168.0.121"; 
const uint16_t websocket_server_port = 8765;

// --- Creación de objetos ---

// Objeto para controlar la pantalla OLED
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
// Objeto para el sensor ultrasónico, con sus pines y distancia máxima
NewPing sonar(TRIG_PIN, ECHO_PIN, MAX_DISTANCE);
// Objeto para controlar el servomotor
Servo servoMotor;
// Objeto cliente para la comunicación WebSocket
WebSocketsClient webSocket;

// --- Variables de estado ---

bool isConnected = false;           // Bandera para saber si el cliente WebSocket está conectado
bool esperando_respuesta = false;   // Bandera para saber si ya se envió "objeto identificado" y se está esperando una respuesta

// --- Constantes del sistema ---

// Ángulos de los movimientos del servo para cada tipo de basura (ej. 0° y 180°)
const byte angulos[2] = {0, 180};
// Comandos esperados para clasificar ('B' para Biodegradable, 'N' para No-biodegradable)
const char comandos[2] = {'B', 'N'};
// Velocidad del movimiento del servo (menor valor = más rápido)
const int velocidad = 10;

// --- Funciones del sistema ---

/**
 * @brief Muestra un mensaje temporal en la pantalla OLED por 5 segundos.
 * @param mensaje El texto que se mostrará en la pantalla.
 */
void mostrarMensajeTemporal(String mensaje) {
    display.clearDisplay();     // Borra todo el contenido anterior
    display.setCursor(0, 0);    // Coloca el cursor en la esquina superior izquierda
    display.println(mensaje);   // Muestra el mensaje
    display.display();          // Actualiza la pantalla física para que el mensaje sea visible
    delay(5000);                // Espera 5 segundos
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
    moverServoSuave(90);  // Regresa el servo a la posición central (90°)
}

/**
 * @brief Procesa los comandos recibidos a través del WebSocket y mueve el servo.
 * @param cmd El comando recibido.
 */
void procesarComando(char cmd) {
    for (int i = 0; i < 2; i++) {
        // Compara el comando recibido con los comandos definidos ('B' o 'N', ignorando mayúsculas/minúsculas)
        if (cmd == comandos[i] || cmd == tolower(comandos[i])) {
            Serial.println("Comando procesado, moviendo servo.");
            moverYVolverSuave(angulos[i]);
            if (cmd == 'B' || cmd == 'b') {
                mostrarMensajeTemporal("Basura\nBiodegradable");
            } else {
                mostrarMensajeTemporal("Basura\nNo-Biodegradable");
            }
            // Enviar confirmación al servidor
            webSocket.sendTXT("recibido");
            Serial.println("Confirmacion 'recibido' enviada al servidor.");
            esperando_respuesta = false; // Reinicia la bandera
            return;
        }
    }
    Serial.println("Comando inválido del servidor.");
}

/**
 * @brief Manejador de eventos del cliente WebSocket.
 * @param type Tipo de evento recibido (conexión, desconexión, mensaje, etc.).
 * @param payload Puntero a los datos del mensaje.
 * @param length Longitud de los datos recibidos.
 */
void conexionWebSocket(WStype_t type, uint8_t * payload, size_t length) {
    switch (type) {
    case WStype_DISCONNECTED:
        Serial.printf("[WSc] Desconectado!\n");
        isConnected = false;
        break;
    case WStype_CONNECTED:
        Serial.printf("[WSc] Conectado a url: %s\n", payload);
        isConnected = true;
        break;
    case WStype_TEXT:
        Serial.printf("[WSc] Mensaje recibido: %s\n", payload);
        if (esperando_respuesta) {
            procesarComando(payload[0]);
        }
        break;
    default:
        break;
    }
}

// --- Configuración inicial (se ejecuta una vez) ---

void setup() {
    Serial.begin(9600);

    // Inicialización de componentes físicos
    servoMotor.attach(SERVO_PIN); // Asocia el objeto servo al pin físico
    moverServoSuave(90); // Mueve el servo a la posición central inicial de forma suave
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN); // Inicia la comunicación I2C
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
    delay(5000);
    display.clearDisplay();
    display.display();

    // Conexión a WiFi
    Serial.println("Conectando a WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
    }
    Serial.println("\nWiFi conectado.");
    Serial.print("Direccion IP: ");
    Serial.println(WiFi.localIP());
    
    // Inicialización del cliente WebSocket
    webSocket.begin(websocket_server_host, websocket_server_port, "/");
    webSocket.onEvent(conexionWebSocket); // Configura el manejador de eventos
    webSocket.setReconnectInterval(5000); // Intenta reconectar cada 5 segundos si se pierde la conexión
}

// --- Bucle principal del programa ---

void loop() {
    webSocket.loop(); // Maneja el flujo de comunicación WebSocket

    static unsigned long ultima_deteccion = 0;
    unsigned long tiempo_actual = millis();
    int distancia = sonar.ping_cm(); // Obtiene la distancia en centímetros
    
    // Lógica de detección: si hay conexión, no estamos esperando respuesta, se detecta un objeto
    // y ha pasado suficiente tiempo desde la última detección para evitar lecturas falsas
    if (isConnected && !esperando_respuesta && distancia > 0 && distancia < MAX_DISTANCE && tiempo_actual - ultima_deteccion > 3000) {
        ultima_deteccion = tiempo_actual;
        
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println("Objeto");
        display.println("Detectado!");
        display.display();
        
        // Envía el mensaje al servidor a través de WebSocket
        webSocket.sendTXT("objeto identificado");
        esperando_respuesta = true;
        Serial.println("Mensaje 'objeto identificado' enviado.");
    }
}