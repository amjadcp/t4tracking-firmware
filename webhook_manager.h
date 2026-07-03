#ifndef WEBHOOK_MANAGER_H
#define WEBHOOK_MANAGER_H

#include <Arduino.h>

void sendWebhookData(String lat, String lng, String speed, String sats, String acc, String gyro, String temp, String lteMod, String lteSim, String lteNet);

#endif // WEBHOOK_MANAGER_H
