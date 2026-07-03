#include "mqtt_manager.h"
#include "config.h"
#include "wifi_manager.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>

static WiFiClientSecure secureClient;
static PubSubClient mqttClient(secureClient);
static unsigned long lastReconnectAttempt = 0;

void mqttSetup() {
    secureClient.setInsecure(); // Skip certificate validation for simplicity. Provide cert if needed.
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
}

static boolean reconnect() {
    if (mqttClient.connect(DEVICE_ID, MQTT_USER, MQTT_PASS)) {
        Serial.println("MQTT connected");
        // We can subscribe here if needed
    }
    return mqttClient.connected();
}

void mqttLoop() {
    if (!isWifiConnected()) return;

    if (!mqttClient.connected()) {
        unsigned long now = millis();
        if (now - lastReconnectAttempt > 5000) {
            lastReconnectAttempt = now;
            if (reconnect()) {
                lastReconnectAttempt = 0;
            }
        }
    } else {
        mqttClient.loop();
    }
}

bool isMqttConnected() {
    return mqttClient.connected();
}

void publishLocation(float lat, float lng, float speed, String statusMsg) {
    if (!mqttClient.connected()) return;

    String topic = String("gps/") + DEVICE_ID + "/location";
    
    // Create JSON payload matching backend: { lat, lng, ts, status }
    // Note: 'ts' will be handled by backend if omitted, or we could pass millis/rtc.
    String payload = "{";
    payload += "\"lat\":" + String(lat, 6) + ",";
    payload += "\"lng\":" + String(lng, 6) + ",";
    payload += "\"status\":\"" + statusMsg + "\"";
    payload += "}";

    mqttClient.publish(topic.c_str(), payload.c_str());
    Serial.println("MQTT Published: " + payload);
}

