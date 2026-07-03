#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include <Arduino.h>

void mqttSetup();
void mqttLoop();
bool isMqttConnected();
void publishLocation(float lat, float lng, float speed, String statusMsg = "moving");

#endif

