#include <WiFi.h>
#include <WebSocketsClient.h>

// Configuración de red
const char* ssid = "IZZI-FA5D";
const char* password = "QJY5MDZYWMLD";

// Configuración del servidor Flask
// IMPORTANTE: Usa la IP local de la computadora donde corre Flask
const char* websocket_server_ip = "192.168.0.121"; 
const uint16_t websocket_port = 5000;
const char* websocket_path = "/socket.io/?EIO=4&transport=websocket"; // El path de SocketIO

WebSocketsClient webSocket;

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch (type) {
    case WStype_DISCONNECTED:
        Serial.printf("[WSc] Desconectado!\n", WStype_DISCONNECTED);
        break;
    case WStype_CONNECTED:
        Serial.printf("[WSc] Conectado a url: %s\n", payload, " ", WStype_CONNECTED);
        break;
    case WStype_TEXT:
        Serial.printf("[WSc] Mensaje recibido: %s\n", payload);
        break;
    default:
        break;
    }
}

void setup() {
    Serial.begin(115200);
    
    // Conexión WiFi
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi conectado.");
    Serial.print("IP del ESP32: ");
    Serial.println(WiFi.localIP());

    // Conexión WebSocket
    webSocket.begin(websocket_server_ip, websocket_port, websocket_path);
    webSocket.onEvent(webSocketEvent);
    // Establece el tiempo de "ping" para mantener la conexión viva
    webSocket.setReconnectInterval(5000); 
}

void loop() {
    webSocket.loop();
    // Aquí puedes añadir más lógica para el ESP32
}