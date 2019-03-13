#include <AsyncMqttClient.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <EEPROM.h>

#define WIFI_SSID "Solomaha"
#define WIFI_PASSWORD "solomakha21"

#define MQTT_HOST IPAddress(192, 168, 1, 230)
#define MQTT_PORT 1883
#define MQTT_ID "room1-secondary_light"
#define MQTT_PASSWORD "fbnkljhjskdvy89vy9c37c3n76y23rcn7y8cyui"

AsyncMqttClient mqttClient;
Ticker mqttReconnectTimer;

WiFiEventHandler wifiConnectHandler;
WiFiEventHandler wifiDisconnectHandler;
Ticker wifiReconnectTimer;

const int relayPin = D1;

char payloadBuffer[6];

void connectToWifi() {
    Serial.println("Connecting to Wi-Fi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void connectToMqtt() {
    Serial.println("Connecting to MQTT...");
    mqttClient.connect();
}

void onWifiConnect(const WiFiEventStationModeGotIP &event) {
    Serial.println("Connected to Wi-Fi.");
    connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected &event) {
    Serial.print("Disconnected from Wi-Fi: ");
    Serial.println(event.reason);

    mqttReconnectTimer.detach();
    wifiReconnectTimer.once(2, connectToWifi);
}

void onMqttConnect(bool) {
    Serial.println("Connected to MQTT.");

    // Subscribe to topics:
    mqttClient.subscribe("switch/room1-secondary_light/set", 0);
    mqttClient.subscribe("device/room1-secondary_light", 0);

    // Send current state
    mqttClient.publish("switch/room1-secondary_light", 0, false, digitalRead(relayPin) == HIGH ? "false" : "true");
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
    Serial.print("Disconnected from MQTT. Reason: ");
    Serial.println((int) reason);

    if (WiFi.isConnected()) {
        mqttReconnectTimer.once(2, connectToMqtt);
    }
}

void onMqttMessage(char *, char *payload, AsyncMqttClientMessageProperties, size_t len, size_t, size_t) {
    payloadBuffer[len] = '\0';
    strncpy(payloadBuffer, payload, len);

    uint8_t newState = HIGH;  // Turn off

    if (strncmp(payloadBuffer, "true", 4) == 0) {
        newState = LOW;  // Turn on
    }

    digitalWrite(relayPin, newState);

    mqttClient.publish("switch/room1-secondary_light", 0, false, payloadBuffer);

    // Save last state to the memory
    EEPROM.put(0, newState);
    EEPROM.commit();
}

void setup() {
    EEPROM.begin(1);    // 1 byte is enough for boolean
    Serial.begin(115200);
    Serial.println();
    Serial.println();

    pinMode(relayPin, OUTPUT);

    uint8_t lastState = HIGH;  // off by default
    EEPROM.get(0, lastState);
    digitalWrite(relayPin, lastState);

    wifiConnectHandler = WiFi.onStationModeGotIP(onWifiConnect);
    wifiDisconnectHandler = WiFi.onStationModeDisconnected(onWifiDisconnect);

    mqttClient.onConnect(onMqttConnect);
    mqttClient.onDisconnect(onMqttDisconnect);
    mqttClient.onMessage(onMqttMessage);
    mqttClient.setServer(MQTT_HOST, MQTT_PORT);
    mqttClient.setClientId(MQTT_ID);
    mqttClient.setCredentials("device", MQTT_PASSWORD);

    connectToWifi();
}

void loop() {}