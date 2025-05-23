// lib/Connectivity/WebServer.h
#pragma once

#include <Arduino.h>
#include <WiFiS3.h>
#include "StateMachine.h"
#include "SafetyMonitor.h"

class WebServer {
public:
    WebServer(StateMachine* stateMachine, SafetyMonitor* safetyMonitor);

    void begin(int port = 80);
    void handleClient();

private:
    WiFiServer _server;
    StateMachine* _stateMachine;
    SafetyMonitor* _safetyMonitor;

    // Page handlers
    void sendHomePage(WiFiClient& client);
    void sendControlPage(WiFiClient& client);
    void sendStatusPage(WiFiClient& client);
    void sendConfigPage(WiFiClient& client);
    void sendDebugPage(WiFiClient& client);
    void send404Page(WiFiClient& client);

    // API handlers
    void handleControlCommand(WiFiClient& client, String command);
    void handleConfigUpdate(WiFiClient& client, String params);

    // Utility functions
    void sendHttpHeader(WiFiClient& client, const char* contentType = "text/html");
    String getSystemStatus();
    String getCurrentDateTime();
};
