#include "wifi_manager.h"
#include "config.h"
#include <WiFi.h>

static unsigned long lastCheckTime = 0;
static bool wasConnected = false;

void wifiSetup() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    WiFi.setAutoReconnect(true); // ESP32 handles reconnection automatically
    Serial.println("Connecting to WiFi...");
}

void wifiLoop() {
    bool currentConnected = (WiFi.status() == WL_CONNECTED);
    
    if (currentConnected && !wasConnected) {
        Serial.println("WiFi Connected!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
    } else if (!currentConnected && wasConnected) {
        Serial.println("WiFi Connection Lost!");
    }
    
    wasConnected = currentConnected;
}

bool isWifiConnected() {
    return WiFi.status() == WL_CONNECTED;
}

