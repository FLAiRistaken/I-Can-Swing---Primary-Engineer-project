// lib/Safety/SafetyMonitor.h
#pragma once

#include <Arduino.h>
#include "Configuration.h"
#include "StateMachine.h"
#include "UltrasonicSensor.h"
#include "PressureSensor.h"

class SafetyMonitor {
public:
    enum SafetyStatus {
        STATUS_OK,         // No safety concerns
        STATUS_WARNING,    // Minor safety concern, doesn't require state change
        STATUS_ERROR,      // Safety issue requiring ERROR state
        STATUS_EMERGENCY   // Critical safety issue requiring EMERGENCY state
    };

    SafetyMonitor(StateMachine* stateMachine,
                 UltrasonicSensor* frontSensor,
                 UltrasonicSensor* rearSensor,
                 PressureSensor* pressureSensor);

    void begin();

    // Main method to check all safety conditions
    SafetyStatus checkSafety();

    // Individual safety checks
    SafetyStatus checkObstacles();
    SafetyStatus checkUserPresence();
    SafetyStatus checkSystemHealth();  // For future expansion

    // Get sensor readings for display/logging
    float getFrontDistance() const;
    float getRearDistance() const;
    bool isUserPresent() const;
    const char* getStatusString() const;
    SafetyStatus getCurrentStatus() const;

private:
    StateMachine* _stateMachine;
    UltrasonicSensor* _frontSensor;
    UltrasonicSensor* _rearSensor;
    PressureSensor* _pressureSensor;

    // Current sensor values
    float _frontDistance;
    float _rearDistance;
    bool _userPresent;
    SafetyStatus _currentStatus;

    // Safety thresholds (using constants from Configuration.h)
    const float CRITICAL_DISTANCE_CM = 10.0f;  // Emergency threshold
    const float WARNING_DISTANCE_CM = OBSTACLE_DISTANCE_CM;  // Warning threshold

    // Handle safety status changes
    void handleSafetyStatus(SafetyStatus newStatus);
};
