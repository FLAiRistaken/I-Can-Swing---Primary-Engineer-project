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
    void exportConfiguration(WiFiClient& client);

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

    // Safety testing state tracking
    bool _safetyTestActive;
    String _currentSafetyTest;
    unsigned long _safetyTestStartTime;
    unsigned long _safetyTestEndTime;
    int _safetyTestStep;
    float _safetyTestThreshold;
    bool _safetyOverrideEnabled;
    unsigned long _safetyOverrideTimeout;

    // Safety test logs
    struct SafetyEventLog {
        unsigned long timestamp;
        String eventType;
        String description;
        String status;
        unsigned long responseTime;
    };
    static const uint8_t MAX_SAFETY_LOGS = 20;
    SafetyEventLog _safetyEventLogs[MAX_SAFETY_LOGS];
    uint8_t _safetyLogIndex;

    // Safety testing methods
    void handleSafetyTestAPI(WiFiClient& client, String command);
    void triggerSafetyEvent(WiFiClient& client, String params);
    void runThresholdTest(WiFiClient& client, String params);
    void measureResponseTime(WiFiClient& client, String params);
    void runAutomatedTestSequence(WiFiClient& client);
    void getSafetyTestStatus(WiFiClient& client);
    void getSafetyEventLogs(WiFiClient& client);
    void toggleSafetyOverride(WiFiClient& client, bool enable);
    void resetSafetyLogs(WiFiClient& client);
    void sendSafetyTestPage(WiFiClient& client);

    // Safety simulation methods
    bool simulateObstacle(float distance, String sensor);
    bool simulateUserDeparture();
    bool simulateMotorStall();

    // Logging methods
    void logSafetyEvent(String eventType, String description, String status, unsigned long responseTime);
    String generateSafetyTestHTML();

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
    void handleMotorTestAPI(WiFiClient& client, String command);

    // Calibration methods
    void startCalibration(WiFiClient& client, String sensorType);
    void saveCalibration(WiFiClient& client);
    void resetCalibration(WiFiClient& client);
    void getCalibrationStatus(WiFiClient& client);
    void updateThreshold(WiFiClient& client, String params);
    void getCalibrationData(WiFiClient& client);

    // Motor testing methods
    void testMotorLeft(WiFiClient& client, String params);
    void testMotorRight(WiFiClient& client, String params);
    void startRampTest(WiFiClient& client, String params);
    void getMotorPosition(WiFiClient& client);
    void testMotorDirection(WiFiClient& client, String params);
    void testMotorSync(WiFiClient& client);
    void stopMotorTest(WiFiClient& client);
    void getMotorTestStatus(WiFiClient& client);

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
