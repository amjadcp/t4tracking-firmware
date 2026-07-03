#ifndef GPS_H
#define GPS_H

#include <Arduino.h>

void gpsSetup();
void gpsLoop();

bool isLocationValid();
float getLatitude();
float getLongitude();
float getSpeedKmph();
int getSatellites();

#endif

