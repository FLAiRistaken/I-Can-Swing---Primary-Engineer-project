// lib/Connectivity/WiFiManager.cpp
#include "WiFiManager.h"

WiFiManager::WiFiManager() : _status(WL_IDLE_STATUS), _lastConnectionCheck(0) {}

bool WiFiManager::begin(const char* ssid, const char* password) {
    // Check for WiFi module
    if (WiFi.status() == WL_NO_MODULE) {
        Serial.println("WiFi: Communication with WiFi module failed!");
        return false;
    }

    Serial.print("WiFi: Connecting to ");
    Serial.println(ssid);

    // Attempt to connect
    _status = WiFi.begin(ssid, password);

    // Wait for connection
    unsigned long startTime = millis();
    while (_status != WL_CONNECTED && millis() - startTime < 10000) {
        delay(500);
        Serial.print(".");
        _status = WiFi.status();
    }

    if (_status == WL_CONNECTED) {
        Serial.println("\nWiFi: Connected successfully!");
        printStatus();
        return true;
    } else {
        Serial.println("\nWiFi: Connection failed!");
        return false;
    }
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::printStatus() {
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal strength (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    Serial.print("Admin Panel: http://");
    Serial.println(WiFi.localIP());
}

String WiFiManager::getLocalIP() {
    return WiFi.localIP().toString();
}
