// lib/Connectivity/WiFiManager.h
#pragma once

#include <Arduino.h>
#include <WiFiS3.h>
#include "Configuration.h"

class WiFiManager {
public:
    WiFiManager();

    bool begin(const char* ssid, const char* password);
    bool isConnected();
    void printStatus();
    String getLocalIP();

private:
    int _status;
    unsigned long _lastConnectionCheck;
    const unsigned long CONNECTION_CHECK_INTERVAL = 30000; // 30 seconds
};
