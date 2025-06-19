// lib/MotorControl/ActuatorDriver.cpp
#include "ActuatorDriver.h"
#include "ExpanderManager.h"

ActuatorDriver::ActuatorDriver(uint8_t forwardPin, uint8_t reversePin, ExpanderManager* expander)
    : _forwardPin(forwardPin), _reversePin(reversePin), _expander(expander),
      _currentDirection(DIRECTION_STOP), _startTime(0), _operationDuration(0), _timedOperation(false) {}

void ActuatorDriver::begin() {
    _expander->pinMode(_forwardPin, OUTPUT);
    _expander->pinMode(_reversePin, OUTPUT);
    stop();
    Serial.println("ActuatorDriver: Initialized");
}

void ActuatorDriver::extend() {
    _expander->digitalWrite(_forwardPin, HIGH);
    _expander->digitalWrite(_reversePin, LOW);
    _currentDirection = DIRECTION_EXTEND;
    Serial.println("ActuatorDriver: Extending");
}

void ActuatorDriver::retract() {
    _expander->digitalWrite(_forwardPin, LOW);
    _expander->digitalWrite(_reversePin, HIGH);
    _currentDirection = DIRECTION_RETRACT;
    Serial.println("ActuatorDriver: Retracting");
}

void ActuatorDriver::stop() {
    _expander->digitalWrite(_forwardPin, LOW);
    _expander->digitalWrite(_reversePin, LOW);
    _currentDirection = DIRECTION_STOP;
    Serial.println("ActuatorDriver: Stopped");
}

ActuatorDriver::Direction ActuatorDriver::getCurrentDirection() const {
    return _currentDirection;
}

void ActuatorDriver::startExtend(unsigned long timeMs) {
    extend();
    _startTime = millis();
    _operationDuration = timeMs;
    _timedOperation = true;
    Serial.print("ActuatorDriver: Timed extend started for ");
    Serial.print(timeMs);
    Serial.println(" ms");
}

void ActuatorDriver::startRetract(unsigned long timeMs) {
    retract();
    _startTime = millis();
    _operationDuration = timeMs;
    _timedOperation = true;
    Serial.print("ActuatorDriver: Timed retract started for ");
    Serial.print(timeMs);
    Serial.println(" ms");
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
