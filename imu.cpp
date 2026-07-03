#include "imu.h"
#include "config.h"
#include <Wire.h>

static void mpuWrite(byte reg, byte val) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}

static int16_t mpuRead(byte reg) {
    Wire.beginTransmission(MPU_ADDR);
    Wire.write(reg);
    Wire.endTransmission(false);
    Wire.requestFrom((uint16_t)MPU_ADDR, (uint8_t)2);
    return (Wire.read() << 8) | Wire.read();
}

void imuSetup() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    
    // MPU6500 init
    mpuWrite(0x6B, 0x00); // wake up
    mpuWrite(0x1C, 0x00); // accel ±2g
    mpuWrite(0x1B, 0x00); // gyro ±250
    Serial.println("MPU6500 OK.");
}

bool isImuConnected() {
    Wire.beginTransmission(MPU_ADDR);
    return Wire.endTransmission() == 0;
}

float getAccX() { return mpuRead(0x3B) / 16384.0; }
float getAccY() { return mpuRead(0x3D) / 16384.0; }
float getAccZ() { return mpuRead(0x3F) / 16384.0; }
float getGyroX() { return mpuRead(0x43) / 131.0; }
float getGyroY() { return mpuRead(0x45) / 131.0; }
float getGyroZ() { return mpuRead(0x47) / 131.0; }
float getTemp() { return mpuRead(0x41) / 333.87 + 21.0; }

