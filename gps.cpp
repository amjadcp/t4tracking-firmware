#include "gps.h"
#include "config.h"
#include <HardwareSerial.h>
#include <TinyGPSPlus.h>

static TinyGPSPlus gps;
static HardwareSerial gpsSerial(2);

static float lastLat = 0;
static float lastLng = 0;
static bool locationValid = false;

void gpsSetup() {
    gpsSerial.begin(9600, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
}

void gpsLoop() {
    while (gpsSerial.available()) {
        char c = gpsSerial.read();
        gps.encode(c);
    }
    
    // Dynamically check if the location is valid and has been updated recently (under 5 seconds)
    locationValid = gps.location.isValid() && (gps.location.age() < 5000);
    
    if (locationValid && gps.location.isUpdated()) {
        lastLat = gps.location.lat();
        lastLng = gps.location.lng();
    }
}

bool isLocationValid() {
    return locationValid;
}

float getLatitude() {
    return lastLat;
}

float getLongitude() {
    return lastLng;
}

float getSpeedKmph() {
    return gps.speed.isValid() ? gps.speed.kmph() : 0.0;
}

int getSatellites() {
    return gps.satellites.isValid() ? gps.satellites.value() : 0;
}

