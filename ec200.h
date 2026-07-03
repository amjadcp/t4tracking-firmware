#ifndef EC200_H
#define EC200_H

#include <Arduino.h>

void ec200Setup();
void ec200Loop();

String sendAT(const char* cmd, int waitMs = 1500);
bool sendSMS(String number, String message);
void handleMissedCall(String number);
void processEC200Line(String line);

extern bool ec200ModuleReady;
extern bool ec200SimReady;
extern bool ec200NetworkReady;

#endif

