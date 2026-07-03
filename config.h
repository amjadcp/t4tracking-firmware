#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include "secrets.h"

// Pin Assignments
#define GPS_RX_PIN 13
#define GPS_TX_PIN 14

#define EC200_RX_PIN 26
#define EC200_TX_PIN 25

#define I2C_SDA_PIN 21
#define I2C_SCL_PIN 22
#define MPU_ADDR    0x68


// Tracking Logic
#define DEVICE_ID     "GPS_MODULE"
#define MQTT_PUBLISH_INTERVAL_MS 5000 // 5 seconds
#define MOVEMENT_THRESHOLD_KMPH  1.0  // km/h threshold to consider "moving"

#endif

