#include "tracker.h"
#include "config.h"
#include "gps.h"
#include "imu.h"
#include "ec200.h"
#include "mqtt_manager.h"
#include "wifi_manager.h"
#include "webhook_manager.h"

enum TrackerMode {
    MODE_INITIAL,
    MODE_TRAVELING,
    MODE_PAUSED,
    MODE_ABSOLUTE_STATIONARY
};

static TrackerMode currentMode = MODE_INITIAL;
static unsigned long lastMqttPublishTime = 0;
static unsigned long stationaryStartTime = 0;
static float prevLat = 0.0;
static float prevLng = 0.0;

static void printSensorTable(float lat, float lng, float speed, bool imuOk) {
    String latStr = String(lat, 6);
    String lngStr = String(lng, 6);
    String speedStr = String(speed, 1) + " km/h";
    String satsStr = (getSatellites() > 0) ? String(getSatellites()) : "E";

    String accStr = imuOk ? (String(getAccX(), 2) + " / " + String(getAccY(), 2) + " / " + String(getAccZ(), 2)) : "E";
    String gyroStr = imuOk ? (String(getGyroX(), 2) + " / " + String(getGyroY(), 2) + " / " + String(getGyroZ(), 2)) : "E";
    String tempStr = imuOk ? (String(getTemp(), 1) + " C") : "E";

    String lteModStr = ec200ModuleReady ? "OK" : "E";
    String lteSimStr = ec200SimReady ? "OK" : "E";
    String lteNetStr = ec200NetworkReady ? "OK" : "E";

    Serial.println("\n=================================================");
    Serial.println("|                  SENSOR DATA                  |");
    Serial.println("=================================================");
    Serial.printf("| %-20s | %-22s |\n", "WiFi Connection", "Connected");
    Serial.printf("| %-20s | %-22s |\n", "MQTT Connection", "Connected");
    Serial.printf("| %-20s | %-22s |\n", "GPS Latitude", latStr.c_str());
    Serial.printf("| %-20s | %-22s |\n", "GPS Longitude", lngStr.c_str());
    Serial.printf("| %-20s | %-22s |\n", "GPS Satellites", satsStr.c_str());
    Serial.printf("| %-20s | %-22s |\n", "GPS Speed", speedStr.c_str());
    Serial.printf("| %-20s | %-22s |\n", "IMU Temperature", tempStr.c_str());
    Serial.printf("| %-20s | %-22s |\n", "IMU Accel X/Y/Z", accStr.c_str());
    Serial.printf("| %-20s | %-22s |\n", "IMU Gyro X/Y/Z", gyroStr.c_str());
    Serial.printf("| %-20s | %-22s |\n", "LTE Module", lteModStr.c_str());
    Serial.printf("| %-20s | %-22s |\n", "LTE SIM", lteSimStr.c_str());
    Serial.printf("| %-20s | %-22s |\n", "LTE Network", lteNetStr.c_str());
    Serial.println("=================================================");

    // Send to Webhook (Errors/timeouts here won't block the rest of the loop)
    sendWebhookData(latStr, lngStr, speedStr, satsStr, accStr, gyroStr, tempStr, lteModStr, lteSimStr, lteNetStr);
}

void trackerSetup() {
    // Initialization of tracker level variables if needed
}

void trackerLoop() {
    unsigned long currentMillis = millis();

    // Check if network is connected. Run check every 5 seconds to avoid flooding console.
    static unsigned long lastConnCheckTime = 0;
    bool wifiConnected = isWifiConnected();
    bool mqttConnected = isMqttConnected();
    
    if (!wifiConnected || !mqttConnected) {
        if (currentMillis - lastConnCheckTime >= 5000) {
            lastConnCheckTime = currentMillis;
            Serial.print("[STATUS] WiFi: ");
            Serial.print(wifiConnected ? "CONNECTED" : "DISCONNECTED");
            Serial.print(" | MQTT: ");
            Serial.println(mqttConnected ? "CONNECTED" : "DISCONNECTED");
        }
        return;
    }

    // Only run tracking if GPS is connected and has a valid fix
    bool gpsOk = isLocationValid();
    bool imuOk = isImuConnected();

    if (!gpsOk) {
        return;
    }

    float lat = getLatitude();
    float lng = getLongitude();
    float speed = getSpeedKmph();

    // Check if the device is moving based on speed or coordinate changes
    bool hasMoved = (speed >= MOVEMENT_THRESHOLD_KMPH) || 
                    (abs(lat - prevLat) > 0.0001) || 
                    (abs(lng - prevLng) > 0.0001);

    switch (currentMode) {
        case MODE_INITIAL: {
            // Initial connection: GPS connects properly for the first time.
            Serial.println("[TRACKER] Initial GPS fix acquired. Sending status: start");
            publishLocation(lat, lng, speed, "start");
            printSensorTable(lat, lng, speed, imuOk);

            prevLat = lat;
            prevLng = lng;
            currentMode = MODE_TRAVELING;
            lastMqttPublishTime = currentMillis;
            stationaryStartTime = 0;
            break;
        }

        case MODE_TRAVELING: {
            if (hasMoved) {
                stationaryStartTime = 0; // Reset stationary timer

                // Send status: ongoing (Exactly every 3 seconds)
                if (currentMillis - lastMqttPublishTime >= MQTT_TRAVELING_PUBLISH_INTERVAL_MS) {
                    lastMqttPublishTime = currentMillis;
                    publishLocation(lat, lng, speed, "ongoing");
                    printSensorTable(lat, lng, speed, imuOk);
                    prevLat = lat;
                    prevLng = lng;
                }
            } else {
                if (stationaryStartTime == 0) {
                    stationaryStartTime = currentMillis;
                    Serial.println("[TRACKER] Device became stationary. Starting 10s timer...");
                }

                // If device remains stationary for more than 10 seconds:
                // STOP sending ongoing status immediately (Mute all data) and switch to Paused Mode
                if (currentMillis - stationaryStartTime > STATIONARY_PAUSED_THRESHOLD_MS) {
                    Serial.println("[TRACKER] Stationary > 10s. Muting data and switching to Paused Mode.");
                    currentMode = MODE_PAUSED;
                } else {
                    // During the 10-second grace period, keep sending ongoing every 3 seconds
                    if (currentMillis - lastMqttPublishTime >= MQTT_TRAVELING_PUBLISH_INTERVAL_MS) {
                        lastMqttPublishTime = currentMillis;
                        publishLocation(lat, lng, speed, "ongoing");
                        printSensorTable(lat, lng, speed, imuOk);
                    }
                }
            }
            break;
        }

        case MODE_PAUSED: {
            // IF device starts moving again before 5 minutes:
            // Switch back to Traveling Mode (Resume ongoing every 3 seconds)
            if (hasMoved) {
                Serial.println("[TRACKER] Movement detected in Paused Mode. Resuming Traveling Mode.");
                currentMode = MODE_TRAVELING;
                stationaryStartTime = 0;
                
                // Immediately publish ongoing and reset intervals
                publishLocation(lat, lng, speed, "ongoing");
                printSensorTable(lat, lng, speed, imuOk);
                lastMqttPublishTime = currentMillis;
                prevLat = lat;
                prevLng = lng;
            } else {
                // IF device remains stationary for a total of 5 minutes:
                // Send status: stop and switch to Absolute Stationary Mode
                if (currentMillis - stationaryStartTime >= STATIONARY_STOP_THRESHOLD_MS) {
                    Serial.println("[TRACKER] Stationary >= 5 minutes. Sending status: stop");
                    publishLocation(lat, lng, speed, "stop");
                    printSensorTable(lat, lng, speed, imuOk);
                    currentMode = MODE_ABSOLUTE_STATIONARY;
                }
            }
            break;
        }

        case MODE_ABSOLUTE_STATIONARY: {
            // DO NOT send any status updates at all (Keep network silent)
            // IF device starts moving from this state:
            // Send status: start and switch back to Traveling Mode (Resume ongoing every 3 seconds)
            if (hasMoved) {
                Serial.println("[TRACKER] Movement detected in Absolute Stationary Mode. Sending status: start");
                publishLocation(lat, lng, speed, "start");
                printSensorTable(lat, lng, speed, imuOk);

                currentMode = MODE_TRAVELING;
                stationaryStartTime = 0;
                lastMqttPublishTime = currentMillis;
                prevLat = lat;
                prevLng = lng;
            }
            break;
        }
    }
}

String buildLocationMessage() {
    String msg = "";

    if (isLocationValid()) {
        msg += "My current location:\n";
        msg += "https://www.google.com/maps?q=";
        msg += String(getLatitude(), 6);
        msg += ",";
        msg += String(getLongitude(), 6);
        msg += "\nLat: " + String(getLatitude(), 6);
        msg += "\nLng: " + String(getLongitude(), 6);
        msg += "\nSats: " + String(getSatellites());
        msg += "\nSpeed: " + String(getSpeedKmph(), 1) + " km/h";
    } else {
        msg += "Location unavailable. No GPS fix.\n";
    }

    msg += "\n--- Motion ---";
    msg += "\nAcc X:" + String(getAccX(), 2);
    msg += " Y:" + String(getAccY(), 2);
    msg += " Z:" + String(getAccZ(), 2);
    msg += "\nGyro X:" + String(getGyroX(), 2);
    msg += " Y:" + String(getGyroY(), 2);
    msg += " Z:" + String(getGyroZ(), 2);
    msg += "\nTemp: " + String(getTemp(), 1) + "C";

    return msg;
}

void handleSerialCommand(String cmd) {
    cmd.trim();
    cmd.toLowerCase();
    Serial.println("CMD: " + cmd);

    if (cmd == "map") {
        sendSMS(SMS_TARGET, buildLocationMessage());
    } else if (cmd == "loc") {
        if (isLocationValid()) {
            Serial.print("Lat: "); Serial.println(getLatitude(), 6);
            Serial.print("Lng: "); Serial.println(getLongitude(), 6);
            Serial.print("Sats: "); Serial.println(getSatellites());
            Serial.print("Speed: "); Serial.println(getSpeedKmph());
        } else {
            Serial.println("No GPS fix.");
        }
    } else if (cmd == "imu") {
        Serial.print("Acc X:"); Serial.print(getAccX());
        Serial.print(" Y:"); Serial.print(getAccY());
        Serial.print(" Z:"); Serial.println(getAccZ());
        Serial.print("Gyro X:"); Serial.print(getGyroX());
        Serial.print(" Y:"); Serial.print(getGyroY());
        Serial.print(" Z:"); Serial.println(getGyroZ());
        Serial.print("Temp: "); Serial.println(getTemp());
    } else if (cmd == "status") {
        Serial.println("Module:  " + String(ec200ModuleReady  ? "OK" : "NOT READY"));
        Serial.println("SIM:     " + String(ec200SimReady     ? "OK" : "NOT READY"));
        Serial.println("Network: " + String(ec200NetworkReady ? "OK" : "NOT READY"));
        Serial.println("GPS:     " + String(isLocationValid() ? "FIX" : "NO FIX"));
        sendAT("AT+CSQ");
        sendAT("AT+CREG?");
        sendAT("AT+COPS?");
    } else if (cmd == "call") {
        String dialCmd = "ATD+91";
        dialCmd += TRIGGER_NUMBER;
        dialCmd += ";";
        sendAT(dialCmd.c_str(), 5000);
    } else if (cmd == "hangup") {
        sendAT("AT+CHUP");
    } else if (cmd == "reset") {
        Serial.println("Resetting EC200U...");
        sendAT("AT+QRST=1");
        delay(5000);
        sendAT("ATE0");
        sendAT("AT+CLIP=1");
        sendAT("AT+CMGF=1");
    } else {
        Serial.println("Commands: map | loc | imu | status | call | hangup | reset");
    }
}

