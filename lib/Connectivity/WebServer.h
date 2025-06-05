// lib/Connectivity/WebServer.h
#pragma once

#include <Arduino.h>
#include <WiFiS3.h>
#include "StateMachine.h"
#include "SafetyMonitor.h"
#include "StepperDriver.h"
#include "RuntimeConfig.h"

class WebServer {
public:
    WebServer(StateMachine* stateMachine, SafetyMonitor* safetyMonitor);
    void setStepperDrivers(StepperDriver* leftStepper, StepperDriver* rightStepper);
    void setDoorActuator(ActuatorDriver* doorActuator);
    void begin(int port = 80);
    void handleClient();

private:
    WiFiServer _server;
    StateMachine* _stateMachine;
    SafetyMonitor* _safetyMonitor;
    StepperDriver* _leftStepper;
    StepperDriver* _rightStepper;
    ActuatorDriver* _doorActuator;

    // ✅ CONSOLIDATED: Single test state structure instead of multiple separate ones
    struct TestState {

        // Motor testing state
        bool motorTestActive;
        String currentMotorTest;
        unsigned long motorTestStartTime;
        int motorTestStep;
        int leftMotorPosition;
        int rightMotorPosition;
        bool motorTestSafetyCheck;

        // Demo state
        bool demoActive;
        String currentDemo;
        unsigned long demoStartTime;
        int demoStep;
        unsigned long demoStepStartTime;
        bool demoSequenceActive;
    } _testState;

    // ✅ SIMPLIFIED: Single log structure for all events
    struct LogEntry {
        unsigned long timestamp;
        String eventType;
        String description;
        String status;
        unsigned long responseTime;
    };
    static const uint8_t MAX_LOGS = 15; // Reduced from 20
    LogEntry _logs[MAX_LOGS];
    uint8_t _logIndex;

    // ✅ STREAMLINED: Core page methods with efficient templates
    void sendPageTemplate(WiFiClient& client, const __FlashStringHelper* title,
                         const __FlashStringHelper* icon, void (WebServer::*contentMethod)(WiFiClient&));
    void sendHomePage(WiFiClient& client);
    void sendControlPage(WiFiClient& client);
    void sendConfigPage(WiFiClient& client);
    void sendMotorTestPage(WiFiClient& client);
    void sendDemoPage(WiFiClient& client);
    void send404Page(WiFiClient& client);

    // ✅ EFFICIENT: Content methods for template system
    void sendHomeContent(WiFiClient& client);
    void sendControlContent(WiFiClient& client);
    void sendConfigContent(WiFiClient& client);
    void sendMotorTestContent(WiFiClient& client);
    void sendDemoContent(WiFiClient& client);

    // ✅ CONSOLIDATED: API handlers
    void handleAPI(WiFiClient& client, String endpoint, String params);
    void handleMotorTestAPI(WiFiClient& client, String command);
    void handleDemoAPI(WiFiClient& client, String command);
    void handleConfigUpdate(WiFiClient& client, String params);
    void handleControlCommand(WiFiClient& client, String command);

    // ✅ MEMORY EFFICIENT: Direct streaming methods
    void sendUnifiedCSS(WiFiClient& client);
    void sendUnifiedJS(WiFiClient& client);
    void sendNavigation(WiFiClient& client);
    void sendCard(WiFiClient& client, const __FlashStringHelper* title, const __FlashStringHelper* icon);
    void sendCardEnd(WiFiClient& client);
    void sendButton(WiFiClient& client, const __FlashStringHelper* text, const char* url, const char* type = "primary");
    void sendFormField(WiFiClient& client, const char* type, const char* name, const char* label, const char* value = "");
    void sendStatusItem(WiFiClient& client, const __FlashStringHelper* label, const __FlashStringHelper* value, const char* statusClass = "");
    void sendStatusItem(WiFiClient& client, const __FlashStringHelper* label, const String& value, const char* statusClass = "");

    // ✅ PRESERVED: All motor testing functionality
    void testMotorLeft(WiFiClient& client, String params);
    void testMotorRight(WiFiClient& client, String params);
    void startRampTest(WiFiClient& client, String params);
    void getMotorPosition(WiFiClient& client);
    void testMotorDirection(WiFiClient& client, String params);
    void testMotorSync(WiFiClient& client);
    void stopMotorTest(WiFiClient& client);
    void getMotorTestStatus(WiFiClient& client);
    bool checkMotorTestSafety();
    void resetMotorPositions();

    // ✅ PRESERVED: Demo functionality
    void startDemo(WiFiClient& client, String params);
    void stopDemo(WiFiClient& client);
    void getDemoStatus(WiFiClient& client);
    void runDemoSequence(String mode);
    void runGentleDemo();
    void runFullFeatureDemo();
    void runSafetyDemo();

    // ✅ ESSENTIAL: Utility methods
    void sendHttpHeader(WiFiClient& client, const char* contentType = "text/html");
    void sendJsonResponse(WiFiClient& client, const __FlashStringHelper* json);
    void sendJsonResponse(WiFiClient& client, const String& json);
    void logEvent(const String& eventType, const String& description, const String& status, unsigned long responseTime = 0);
    void exportConfiguration(WiFiClient& client);
    String getCurrentDateTime();
    String getSystemStatus();
};
