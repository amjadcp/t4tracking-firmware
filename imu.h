#ifndef IMU_H
#define IMU_H

#include <Arduino.h>

void imuSetup();

float getAccX();
float getAccY();
float getAccZ();

float getGyroX();
float getGyroY();
float getGyroZ();

float getTemp();

bool isImuConnected();

#endif

