#include "config.h"
#include "gps.h"
#include "imu.h"
#include "ec200.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "tracker.h"

static String serialBuffer = "";

void setup() {
    Serial.begin(115200);
    delay(5000);

    Serial.println("=== Location Tracker ===");
    Serial.println("Commands: map | loc | imu | status | call | hangup | reset");
    Serial.println("========================================");

    gpsSetup();
    imuSetup();
    ec200Setup();
    wifiSetup();
    mqttSetup();
    trackerSetup();

    Serial.println("Setup complete.");
}

void loop() {
    gpsLoop();
    ec200Loop();
    wifiLoop();
    mqttLoop();
    trackerLoop();

    // Serial commands
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            if (serialBuffer.length() > 0) {
                handleSerialCommand(serialBuffer);
                serialBuffer = "";
            }
        } else {
            serialBuffer += c;
        }
    }
}

