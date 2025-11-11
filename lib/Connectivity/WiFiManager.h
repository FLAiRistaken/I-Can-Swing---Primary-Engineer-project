// lib/Connectivity/WiFiManager.h
#pragma once

#include <WiFi.h>

class WiFiManager {
public:
    WiFiManager();

    bool begin(const char* ssid, const char* password);
    void update();
    bool checkAndReconnect();
    bool isConnected();
    void printStatus();
    String getLocalIP();
    void setReconnectInterval(unsigned long interval);

private:
    char _ssid[32];                   // Store SSID for reconnection
    char _password[64];               // Store password for reconnection
    int _status;
    unsigned long _lastConnectionCheck;
    unsigned long _reconnectInterval;
    unsigned long _reconnectAttemptTime;
    bool _reconnecting;
};
