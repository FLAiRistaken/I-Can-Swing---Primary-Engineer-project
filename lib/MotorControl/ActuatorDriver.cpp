// lib/MotorControl/ActuatorDriver.cpp
#include "ActuatorDriver.h"
#include "ExpanderManager.h"
#include "Debug.h"

ActuatorDriver::ActuatorDriver(uint8_t forwardPin, uint8_t reversePin, ExpanderManager* expander)
    : _forwardPin(forwardPin), _reversePin(reversePin), _expander(expander),
      _currentDirection(DIRECTION_STOP), _startTime(0), _operationDuration(0), _timedOperation(false) {}

void ActuatorDriver::begin() {
    _expander->pinMode(_forwardPin, OUTPUT);
    _expander->pinMode(_reversePin, OUTPUT);
    stop();
    DEBUG_PRINTLN("ActuatorDriver: Initialized");
}

void ActuatorDriver::extend() {
    _expander->digitalWrite(_forwardPin, HIGH);
    _expander->digitalWrite(_reversePin, LOW);
    _currentDirection = DIRECTION_EXTEND;
    DEBUG_PRINTLN("ActuatorDriver: Extending");
}

void ActuatorDriver::retract() {
    _expander->digitalWrite(_forwardPin, LOW);
    _expander->digitalWrite(_reversePin, HIGH);
    _currentDirection = DIRECTION_RETRACT;
    DEBUG_PRINTLN("ActuatorDriver: Retracting");
}

void ActuatorDriver::stop() {
    _expander->digitalWrite(_forwardPin, LOW);
    _expander->digitalWrite(_reversePin, LOW);
    _currentDirection = DIRECTION_STOP;
    DEBUG_PRINTLN("ActuatorDriver: Stopped");
}

ActuatorDriver::Direction ActuatorDriver::getCurrentDirection() const {
    return _currentDirection;
}

void ActuatorDriver::startExtend(unsigned long timeMs) {
    extend();
    _startTime = millis();
    _operationDuration = timeMs;
    _timedOperation = true;
    DEBUG_PRINT("ActuatorDriver: Timed extend started for ");
    DEBUG_PRINT(timeMs);
    DEBUG_PRINTLN(" ms");
}

void ActuatorDriver::startRetract(unsigned long timeMs) {
    retract();
    _startTime = millis();
    _operationDuration = timeMs;
    _timedOperation = true;
    DEBUG_PRINT("ActuatorDriver: Timed retract started for ");
    DEBUG_PRINT(timeMs);
    DEBUG_PRINTLN(" ms");
}

void ActuatorDriver::update() {
    if (!_timedOperation || _currentDirection == DIRECTION_STOP) {
        return;
    }

    if (millis() - _startTime >= _operationDuration) {
        // Operation complete
        stop();
        _timedOperation = false;

        // Test mode feedback (always available, now the only output)
        if (_currentDirection == DIRECTION_EXTEND) {
            Serial.println("ActuatorDriver: Door open operation complete.");
        } else if (_currentDirection == DIRECTION_RETRACT) {
            Serial.println("ActuatorDriver: Door close operation complete.");
        }
    }
}


bool ActuatorDriver::isMoving() const {
    return _currentDirection != DIRECTION_STOP;
}
