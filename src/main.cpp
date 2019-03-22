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

const uint8_t relayPin = D1;

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
    digitalWrite(LED_BUILTIN, LOW);

    connectToMqtt();
}

void onWifiDisconnect(const WiFiEventStationModeDisconnected &event) {
    Serial.print("Disconnected from Wi-Fi: ");
    Serial.println(event.reason);
    digitalWrite(LED_BUILTIN, HIGH);

    mqttReconnectTimer.detach();
    wifiReconnectTimer.once(2, connectToWifi);
}

void onMqttConnect(bool) {
    Serial.println("Connected to MQTT.");
    digitalWrite(LED_BUILTIN, LOW);

    // Subscribe to topics:
    mqttClient.subscribe("switch/room1-secondary_light/set", 0);
    mqttClient.subscribe("switch/room1-secondary_light/toggle", 0);
    mqttClient.subscribe("device/room1-secondary_light", 0);

    // Send current state
    mqttClient.publish("switch/room1-secondary_light", 0, false, digitalRead(relayPin) == HIGH ? "false" : "true");
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
    Serial.print("Disconnected from MQTT. Reason: ");
    Serial.println((int) reason);
    digitalWrite(LED_BUILTIN, HIGH);

    if (WiFi.isConnected()) {
        mqttReconnectTimer.once(2, connectToMqtt);
    }
}

void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties, size_t len, size_t, size_t) {
    uint8_t newState = HIGH;

    if (strcmp(topic, "switch/room1-secondary_light/toggle") == 0) {
        if (digitalRead(relayPin) == HIGH) {
            newState = LOW;
        }

        mqttClient.publish("switch/room1-secondary_light", 0, false, newState == HIGH ? "false" : "true");
    } else {
        payloadBuffer[len] = '\0';
        strncpy(payloadBuffer, payload, len);

        if (strncmp(payloadBuffer, "true", 4) == 0) {
            newState = LOW;  // Turn on
        }

        mqttClient.publish("switch/room1-secondary_light", 0, false, payloadBuffer);
    }

    digitalWrite(relayPin, newState);

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