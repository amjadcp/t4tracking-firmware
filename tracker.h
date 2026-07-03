#ifndef TRACKER_H
#define TRACKER_H

#include <Arduino.h>

void trackerSetup();
void trackerLoop();
String buildLocationMessage();
void handleSerialCommand(String cmd);

#endif

