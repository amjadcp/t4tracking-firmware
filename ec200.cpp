#include "ec200.h"
#include "config.h"
#include "tracker.h"
#include <HardwareSerial.h>

static HardwareSerial ec200(1);
static String ec200Buffer = "";

bool ec200ModuleReady = false;
bool ec200SimReady = false;
bool ec200NetworkReady = false;

static bool callInProgress = false;
static String currentCaller = "";
static unsigned long callStartTime = 0;
static bool smsSent = false;

String sendAT(const char* cmd, int waitMs) {
    Serial.print(">> "); Serial.println(cmd);
    ec200.print(cmd);
    ec200.print("\r\n");

    String response = "";
    unsigned long t = millis();
    while (millis() - t < waitMs) {
        while (ec200.available()) {
            char c = ec200.read();
            response += c;
        }
        if (response.indexOf("OK") >= 0 || response.indexOf("ERROR") >= 0) break;
    }
    response.trim();
    Serial.println(response.length() > 0 ? response : "(no response)");
    return response;
}

bool sendSMS(String number, String message) {
    Serial.println("Sending SMS to " + number);
    sendAT("AT+CMGF=1");

    ec200.print("AT+CMGS=\"");
    ec200.print(number);
    ec200.print("\"\r\n");
    delay(1500);

    String prompt = "";
    unsigned long t = millis();
    while (millis() - t < 2000) {
        while (ec200.available()) prompt += (char)ec200.read();
    }

    if (prompt.indexOf(">") < 0) {
        Serial.println("No > prompt. SMS aborted.");
        ec200.write(27);
        return false;
    }

    ec200.print(message);
    delay(500);
    ec200.write(26); // Ctrl+Z

    String confirm = "";
    t = millis();
    while (millis() - t < 8000) {
        while (ec200.available()) confirm += (char)ec200.read();
        if (confirm.indexOf("+CMGS:") >= 0) break;
        if (confirm.indexOf("ERROR") >= 0) break;
    }

    if (confirm.indexOf("+CMGS:") >= 0) {
        Serial.println("SMS sent!");
        return true;
    }
    Serial.println("SMS failed!");
    return false;
}

void handleMissedCall(String number) {
    Serial.println("Missed call from: " + number);
    if (!smsSent) {
        String msg = buildLocationMessage();
        bool ok = sendSMS(SMS_TARGET, msg);
        if (ok) smsSent = true;
    } else {
        Serial.println("SMS already sent.");
    }
}

void processEC200Line(String line) {
    line.trim();
    if (line.length() == 0) return;
    Serial.println("[EC200] " + line);

    if (line == "RDY" || line.indexOf("READY") >= 0) {
        ec200ModuleReady = true;
    }
    if (line.indexOf("+CPIN: READY") >= 0) {
        ec200SimReady = true;
        Serial.println("SIM ready.");
    }
    if (line.indexOf("+CPIN: NO SIM") >= 0) {
        Serial.println("ERROR: No SIM!");
    }
    if (line.indexOf("+CREG: 0,1") >= 0 || line.indexOf("+CREG: 0,5") >= 0) {
        ec200NetworkReady = true;
        Serial.println("Network registered.");
    }
    if (line.indexOf("+CREG: 0,0") >= 0) {
        ec200NetworkReady = false;
        Serial.println("WARNING: Network lost!");
    }
    if (line == "RING") {
        if (!callInProgress) {
            callInProgress = true;
            callStartTime = millis();
            smsSent = false;
            Serial.println("RING detected!");
        }
    }
    if (line.indexOf("+CLIP:") >= 0) {
        int start = line.indexOf("\"") + 1;
        int end   = line.indexOf("\"", start);
        if (start > 0 && end > start) {
            currentCaller = line.substring(start, end);
            currentCaller.trim();
            Serial.println("Caller: " + currentCaller);
        }
    }
    if (line.indexOf("NO CARRIER") >= 0) {
        Serial.println("Caller hung up.");
        if (callInProgress) {
            if (currentCaller.indexOf(TRIGGER_NUMBER) >= 0) {
                handleMissedCall(currentCaller);
            } else if (currentCaller.length() > 0) {
                Serial.println("Unknown caller — ignored.");
            } else {
                Serial.println("No CLIP — ignored.");
            }
        }
        callInProgress = false;
        currentCaller = "";
        smsSent = false;
    }
}

void ec200Setup() {
    ec200.begin(115200, SERIAL_8N1, EC200_RX_PIN, EC200_TX_PIN);
    delay(1000); // Give UART time to stabilize
    
    sendAT("ATE1", 1500);
    sendAT("AT");
    sendAT("ATE0");
    sendAT("AT+CLIP=1");
    sendAT("AT+CMGF=1");
    sendAT("AT+QINDCFG=\"ring\",1");

    String cpin = sendAT("AT+CPIN?");
    if (cpin.indexOf("READY") >= 0) {
        ec200SimReady = true;
        ec200ModuleReady = true;
    }
    String creg = sendAT("AT+CREG?");
    if (creg.indexOf(",1") >= 0 || creg.indexOf(",5") >= 0) {
        ec200NetworkReady = true;
    }
}

void ec200Loop() {
    while (ec200.available()) {
        char c = ec200.read();
        Serial.write(c);
        ec200Buffer += c;
        if (c == '\n') {
            processEC200Line(ec200Buffer);
            ec200Buffer = "";
        }
    }

    if (callInProgress && millis() - callStartTime > 30000) {
        Serial.println("Call timeout — resetting.");
        callInProgress = false;
        currentCaller = "";
        smsSent = false;
    }
}

