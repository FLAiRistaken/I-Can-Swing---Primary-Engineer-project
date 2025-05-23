// lib/Connectivity/WebServer.h
#pragma once

#include <Arduino.h>
#include <WiFiS3.h>
#include "StateMachine.h"
#include "SafetyMonitor.h"
#include "StepperDriver.h"


class WebServer {
public:
    WebServer(StateMachine* stateMachine, SafetyMonitor* safetyMonitor);

    void setStepperDrivers(StepperDriver* leftStepper, StepperDriver* rightStepper);

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

    // Motor testing components
    StepperDriver* _leftStepper;
    StepperDriver* _rightStepper;

    // Motor testing state tracking
    bool _motorTestActive;
    String _currentMotorTest;
    unsigned long _motorTestStartTime;
    int _motorTestStep;
    int _leftMotorPosition;
    int _rightMotorPosition;
    bool _motorTestSafetyCheck;

    // Test parameters
    struct MotorTestParams {
        uint16_t minSpeed;
        uint16_t maxSpeed;
        uint16_t rampIncrement;
        unsigned long rampInterval;
        int testSteps;
        bool testDirection;
    } _testParams;

    // Page handlers
    void sendHomePage(WiFiClient& client);
    void sendControlPage(WiFiClient& client);
    void sendStatusPage(WiFiClient& client);
    void sendConfigPage(WiFiClient& client);
    void sendDebugPage(WiFiClient& client);
    void sendCalibrationPage(WiFiClient& client);
    void sendMotorTestPage(WiFiClient& client);
    void send404Page(WiFiClient& client);

    // API handlers
    void handleControlCommand(WiFiClient& client, String command);
    void handleConfigUpdate(WiFiClient& client, String params);
    void handleCalibrationAPI(WiFiClient& client, String command);
    void handleMotorTestAPI(WifiClient& client, String command);

    // Calibration methods
    void startCalibration(WiFiClient& client, String sensorType);
    void saveCalibration(WiFiClient& client);
    void resetCalibration(WiFiClient& client);
    void getCalibrationStatus(WiFiClient& client);
    void updateThreshold(WiFiClient& client, String params);
    void getCalibrationData(WiFiClient& client);

    // Motor testing methods
    void handleMotorTestAPI(WiFiClient& client, String command);
    void testMotorLeft(WiFiClient& client, String params);
    void testMotorRight(WiFiClient& client, String params);
    void startRampTest(WiFiClient& client, String params);
    void getMotorPosition(WiFiClient& client);
    void testMotorDirection(WiFiClient& client, String params);
    void testMotorSync(WiFiClient& client);
    void stopMotorTest(WiFiClient& client);
    void getMotorTestStatus(WiFiClient& client);
    void sendMotorTestPage(WiFiClient& client);

    // Safety interlocks
    bool checkMotorTestSafety();
    void resetMotorPositions();

    // Utility functions
    void sendHttpHeader(WiFiClient& client, const char* contentType = "text/html");
    void sendJsonResponse(WiFiClient& client, String jsonData);
    String getSystemStatus();
    String getCurrentDateTime();
    String generateCalibrationWizardHTML();
    String generateMotorTestHTML();
};
