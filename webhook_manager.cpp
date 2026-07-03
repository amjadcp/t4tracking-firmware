#include "webhook_manager.h"
#include "config.h"
#include "wifi_manager.h"
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

void sendWebhook(String jsonPayload) {
#ifdef WEBHOOK_URL
    if (strlen(WEBHOOK_URL) == 0) {
        return; // Webhook URL is empty, feature disabled
    }
    if (!isWifiConnected()) {
        return; // No WiFi connection, skip sending
    }

    String url = String(WEBHOOK_URL);
    bool isHttps = url.startsWith("https://");

    HTTPClient http;

    if (isHttps) {
        WiFiClientSecure secureClient;
        secureClient.setInsecure(); // Skip certificate verification for robustness
        if (http.begin(secureClient, url)) {
            http.setTimeout(1500); // 1.5s timeout - will not block the main loop significantly
            http.addHeader("Content-Type", "application/json");
            
            int httpResponseCode = http.POST(jsonPayload);
            if (httpResponseCode > 0) {
                Serial.print("[Webhook] HTTPS POST Response: ");
                Serial.println(httpResponseCode);
            } else {
                Serial.print("[Webhook] HTTPS POST Failed: ");
                Serial.println(http.errorToString(httpResponseCode).c_str());
            }
            http.end();
        } else {
            Serial.println("[Webhook] Failed to initialize HTTPS connection.");
        }
    } else {
        WiFiClient client;
        if (http.begin(client, url)) {
            http.setTimeout(1500); // 1.5s timeout
            http.addHeader("Content-Type", "application/json");

            int httpResponseCode = http.POST(jsonPayload);
            if (httpResponseCode > 0) {
                Serial.print("[Webhook] HTTP POST Response: ");
                Serial.println(httpResponseCode);
            } else {
                Serial.print("[Webhook] HTTP POST Failed: ");
                Serial.println(http.errorToString(httpResponseCode).c_str());
            }
            http.end();
        } else {
            Serial.println("[Webhook] Failed to initialize HTTP connection.");
        }
    }
#endif
}
