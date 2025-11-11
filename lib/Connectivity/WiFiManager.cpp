// lib/Connectivity/WiFiManager.cpp
#include "WiFiManager.h"

WiFiManager::WiFiManager()
    : _status(WL_IDLE_STATUS),
      _lastConnectionCheck(0),
      _reconnectInterval(60000),  // Check every 60 seconds
      _reconnectAttemptTime(0),
      _reconnecting(false) {
}

bool WiFiManager::begin(const char* ssid, const char* password) {
    // Store credentials for reconnection
    strncpy(_ssid, ssid, sizeof(_ssid) - 1);
    strncpy(_password, password, sizeof(_password) - 1);
    _ssid[sizeof(_ssid) - 1] = '\0';
    _password[sizeof(_password) - 1] = '\0';

    // Check for WiFi module
    if (WiFi.status() == WL_NO_MODULE) {
        Serial.println("WiFi: Communication with WiFi module failed!");
        return false;
    }

    Serial.print("WiFi: Connecting to ");
    Serial.println(ssid);

    // Multiple connection attempts at startup
    for (int attempt = 1; attempt <= 5; attempt++) {
        Serial.print("WiFi: Connection attempt ");
        Serial.print(attempt);
        Serial.println("/5");

        WiFi.disconnect();
        delay(1000);

        _status = WiFi.begin(ssid, password);

        // Wait for connection
        unsigned long startTime = millis();
        while (_status != WL_CONNECTED && millis() - startTime < 20000) {
            delay(1000);
            Serial.print(".");
            _status = WiFi.status();
        }

        if (_status == WL_CONNECTED) {
            Serial.println("\nWiFi: Connected successfully!");
            printStatus();
            _lastConnectionCheck = millis();
            return true;
        }

        Serial.println("\nWiFi: Connection failed, retrying...");
        delay(2000);
    }

    Serial.println("WiFi: All connection attempts failed!");
    return false;
}

void WiFiManager::update() {
    unsigned long currentTime = millis();

    // Check connection status less frequently - every 30 seconds
    if (currentTime - _lastConnectionCheck >= 30000) {
        _lastConnectionCheck = currentTime;
        _status = WiFi.status();

        if (_status != WL_CONNECTED) {
            Serial.println("WiFi: Connection lost!");

            // Attempt reconnection if it's time (every 60 seconds)
            if (!_reconnecting && (currentTime - _reconnectAttemptTime >= _reconnectInterval)) {
                checkAndReconnect();
            }
        }
    }
}

bool WiFiManager::checkAndReconnect() {
    if (_reconnecting) {
        return false; // Already attempting reconnection
    }

    _reconnecting = true;
    _reconnectAttemptTime = millis();

    Serial.println("WiFi: Attempting reconnection...");

    // More thorough disconnection process
    WiFi.disconnect();
    WiFi.end();
    delay(2000);  // Longer delay for complete reset

    // Multiple reconnection attempts with progressive delays
    for (int attempt = 1; attempt <= 3; attempt++) {
        Serial.print("WiFi: Reconnection attempt ");
        Serial.print(attempt);
        Serial.println("/3");

        _status = WiFi.begin(_ssid, _password);

        // Wait for connection with timeout
        unsigned long startTime = millis();
        while (_status != WL_CONNECTED && millis() - startTime < 15000) {
            delay(1000);  // Longer delay between checks
            Serial.print(".");
            _status = WiFi.status();
        }

        if (_status == WL_CONNECTED) {
            Serial.println("\nWiFi: Reconnected successfully!");
            printStatus();
            _reconnecting = false;
            return true;
        }

        Serial.println("\nWiFi: Attempt failed");
        if (attempt < 3) {
            delay(5000 * attempt);  // Progressive delay: 5s, 10s, 15s
        }
    }

    _reconnecting = false;
    Serial.println("WiFi: All reconnection attempts failed!");
    return false;
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::setReconnectInterval(unsigned long interval) {
    _reconnectInterval = interval;
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
