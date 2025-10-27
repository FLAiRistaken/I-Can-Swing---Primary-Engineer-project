// lib/Connectivity/WebServer.h
#pragma once

#include <WiFi.h>
#include <WiFiServer.h>
#include "StateMachine.h"
#include "SafetyMonitor.h"
#include "RuntimeConfig.h"

class WebServer {
public:
    WebServer(StateMachine* stateMachine, SafetyMonitor* safetyMonitor);
    void begin(int port = 80);
    void handleClient();

private:
    WiFiServer _server;
    StateMachine* _stateMachine;
    SafetyMonitor* _safetyMonitor;

    // Simple logging for troubleshooting
    struct LogEntry {
        unsigned long timestamp;
        String event;
        String status;
    };
    static const uint8_t MAX_LOGS = 5;
    LogEntry _logs[MAX_LOGS];
    uint8_t _logIndex;

    // Core page handlers
    void sendHomePage(WiFiClient& client);
    void sendControlPage(WiFiClient& client);
    void sendConfigPage(WiFiClient& client);
    void send404Page(WiFiClient& client);

    void sendPhysicsConfigSection(WiFiClient& client);
    void sendLiveMonitorSection(WiFiClient& client);
    bool processAllConfigParams(String params, RuntimeConfig& config);
    void logResponseTime(unsigned long startTime);


    // API handlers
    void handleControlAPI(WiFiClient& client, String command);
    void handleConfigAPI(WiFiClient& client, String params);
    void handleStatusAPI(WiFiClient& client);

    // Utility methods
    void sendHttpHeader(WiFiClient& client, const char* contentType = "text/html");
    void sendJsonResponse(WiFiClient& client, const String& json);
    void sendPageHeader(WiFiClient& client, const String& title);
    void sendPageFooter(WiFiClient& client);
    void sendSimpleCSS(WiFiClient& client);

    void logEvent(const String& event, const String& status);
    String getOptimizedAjaxScript();
    String getRecentLogsOptimized();
    String getSensorData();
    String getSystemStatus();
};
