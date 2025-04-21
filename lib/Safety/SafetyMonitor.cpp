// lib/Safety/SafetyMonitor.cpp
#include "SafetyMonitor.h"

SafetyMonitor::SafetyMonitor(StateMachine* stateMachine,
                           UltrasonicSensor* frontSensor,
                           UltrasonicSensor* rearSensor,
                           PressureSensor* pressureSensor)
    : _stateMachine(stateMachine),
      _frontSensor(frontSensor),
      _rearSensor(rearSensor),
      _pressureSensor(pressureSensor),
      _frontDistance(0.0f),
      _rearDistance(0.0f),
      _userPresent(false),
      _currentStatus(STATUS_OK) {}

void SafetyMonitor::begin() {
    Serial.println("SafetyMonitor: Initialized");
    // Initial safety check
    checkSafety();
}

SafetyMonitor::SafetyStatus SafetyMonitor::checkSafety() {
    // Run all safety checks and determine most critical status
    SafetyStatus obstacleStatus = checkObstacles();
    SafetyStatus userStatus = checkUserPresence();
    SafetyStatus systemStatus = checkSystemHealth();

    // Determine most severe status
    SafetyStatus newStatus = STATUS_OK;

    if (obstacleStatus == STATUS_EMERGENCY ||
        userStatus == STATUS_EMERGENCY ||
        systemStatus == STATUS_EMERGENCY) {
        newStatus = STATUS_EMERGENCY;
    } else if (obstacleStatus == STATUS_ERROR ||
               userStatus == STATUS_ERROR ||
               systemStatus == STATUS_ERROR) {
        newStatus = STATUS_ERROR;
    } else if (obstacleStatus == STATUS_WARNING ||
               userStatus == STATUS_WARNING ||
               systemStatus == STATUS_WARNING) {
        newStatus = STATUS_WARNING;
    }

    // Handle status changes if needed
    if (newStatus != _currentStatus) {
        handleSafetyStatus(newStatus);
        _currentStatus = newStatus;
    }

    return _currentStatus;
}

SafetyMonitor::SafetyStatus SafetyMonitor::checkObstacles() {
    // Get distance readings
    _frontDistance = _frontSensor->measureDistance();
    _rearDistance = _rearSensor->measureDistance();

    // Check for critical proximity (EMERGENCY)
    if ((_frontDistance > 0 && _frontDistance < CRITICAL_DISTANCE_CM) ||
        (_rearDistance > 0 && _rearDistance < CRITICAL_DISTANCE_CM)) {
        Serial.println("SafetyMonitor: CRITICAL - Object extremely close!");
        return STATUS_EMERGENCY;
    }

    // Check for obstacles (ERROR)
    if ((_frontDistance > CRITICAL_DISTANCE_CM && _frontDistance < WARNING_DISTANCE_CM) ||
        (_rearDistance > CRITICAL_DISTANCE_CM && _rearDistance < WARNING_DISTANCE_CM)) {
        Serial.print("SafetyMonitor: WARNING - Object detected at ");
        Serial.print((_frontDistance < WARNING_DISTANCE_CM) ? _frontDistance : _rearDistance);
        Serial.println(" cm");
        return STATUS_ERROR;
    }

    return STATUS_OK;
}

SafetyMonitor::SafetyStatus SafetyMonitor::checkUserPresence() {
    // Check if user is present in the swing
    _userPresent = _pressureSensor->isOccupied();

    // If swinging and user suddenly disappears, trigger emergency
    if (!_userPresent &&
        _stateMachine->getCurrentState() == StateMachine::STATE_SWINGING) {
        Serial.println("SafetyMonitor: EMERGENCY - User absence during swinging");
        return STATUS_EMERGENCY;
    }

    return STATUS_OK;
}

SafetyMonitor::SafetyStatus SafetyMonitor::checkSystemHealth() {
    // Check if the system has been running too long without user interaction
    static unsigned long lastActivityTime = millis();
    static unsigned long systemStartTime = millis();

    // Future enhancement: check battery voltage
    // Future enhancement: check motor current
    // Future enhancement: monitor communication errors

    // Example of a system health check:
    unsigned long currentTime = millis();
    if (currentTime - systemStartTime > 3600000) { // 1 hour
        // Implement periodic system check after 1 hour of operation
        // This would check for system fatigue or overheating
        Serial.println("SafetyMonitor: Performing 1-hour system health check");
        systemStartTime = currentTime; // Reset for next hour
    }

    return STATUS_OK;
}


void SafetyMonitor::handleSafetyStatus(SafetyStatus newStatus) {
    // Avoid redundant state transitions
    StateMachine::State currentState = _stateMachine->getCurrentState();

    switch (newStatus) {
        case STATUS_EMERGENCY:
            if (currentState != StateMachine::STATE_EMERGENCY) {
                Serial.println("SafetyMonitor: Triggering EMERGENCY event");
                _stateMachine->processEvent(StateMachine::EVENT_EMERGENCY);
            }
            break;

        case STATUS_ERROR:
            if (currentState != StateMachine::STATE_ERROR &&
                currentState != StateMachine::STATE_EMERGENCY) {
                Serial.println("SafetyMonitor: Triggering ERROR event");
                _stateMachine->processEvent(StateMachine::EVENT_OBSTACLE_DETECTED);
            }
            break;

        case STATUS_WARNING:
            // Warnings don't change state currently
            Serial.println("SafetyMonitor: Warning condition - no state change");
            break;

        case STATUS_OK:
            // No actions needed for OK status
            break;
    }
}

float SafetyMonitor::getFrontDistance() const {
    return _frontDistance;
}

float SafetyMonitor::getRearDistance() const {
    return _rearDistance;
}

bool SafetyMonitor::isUserPresent() const {
    return _userPresent;
}

SafetyMonitor::SafetyStatus SafetyMonitor::getCurrentStatus() const {
    return _currentStatus;
}

const char* SafetyMonitor::getStatusString() const {
    switch (_currentStatus) {
        case STATUS_OK:
            return "SAFE";
        case STATUS_WARNING:
            return "WARNING";
        case STATUS_ERROR:
            return "ERROR";
        case STATUS_EMERGENCY:
            return "EMERGENCY";
        default:
            return "UNKNOWN";
    }
}
