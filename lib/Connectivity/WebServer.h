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

    // Calibration state tracking
    bool _calibrationActive;
    unsigned long _calibrationStartTime;
    int _calibrationStep;
    String _currentSensorCalibrating;

    // Page handlers
    void sendHomePage(WiFiClient& client);
    void sendControlPage(WiFiClient& client);
    void sendStatusPage(WiFiClient& client);
    void sendConfigPage(WiFiClient& client);
    void sendDebugPage(WiFiClient& client);
    void sendCalibrationPage(WiFiClient& client);  // New
    void send404Page(WiFiClient& client);

    // API handlers
    void handleControlCommand(WiFiClient& client, String command);
    void handleConfigUpdate(WiFiClient& client, String params);
    void handleCalibrationAPI(WiFiClient& client, String command);  // New

    // Calibration methods
    void startCalibration(WiFiClient& client, String sensorType);
    void saveCalibration(WiFiClient& client);
    void resetCalibration(WiFiClient& client);
    void getCalibrationStatus(WiFiClient& client);
    void updateThreshold(WiFiClient& client, String params);
    void getCalibrationData(WiFiClient& client);

    // Utility functions
    void sendHttpHeader(WiFiClient& client, const char* contentType = "text/html");
    void sendJsonResponse(WiFiClient& client, String jsonData);
    String getSystemStatus();
    String getCurrentDateTime();
    String generateCalibrationWizardHTML();
};
