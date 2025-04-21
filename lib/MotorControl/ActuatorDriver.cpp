// lib/MotorControl/ActuatorDriver.cpp
#include "ActuatorDriver.h"
#include "StateMachine.h"

ActuatorDriver::ActuatorDriver(uint8_t forwardPin, uint8_t reversePin, StateMachine* stateMachine)
    : _forwardPin(forwardPin), _reversePin(reversePin), _stateMachine(stateMachine),
      _currentDirection(DIRECTION_STOP), _startTime(0), _operationDuration(0), _timedOperation(false) {}

void ActuatorDriver::begin() {
    pinMode(_forwardPin, OUTPUT);
    pinMode(_reversePin, OUTPUT);
    digitalWrite(_forwardPin, LOW);
    digitalWrite(_reversePin, LOW);
    Serial.println("ActuatorDriver: Initialized");
}

void ActuatorDriver::extend() {
    digitalWrite(_forwardPin, HIGH);
    digitalWrite(_reversePin, LOW);
    _currentDirection = DIRECTION_EXTEND;
    Serial.println("ActuatorDriver: Extending");
}

void ActuatorDriver::retract() {
    digitalWrite(_forwardPin, LOW);
    digitalWrite(_reversePin, HIGH);
    _currentDirection = DIRECTION_RETRACT;
    Serial.println("ActuatorDriver: Retracting");
}

void ActuatorDriver::stop() {
    digitalWrite(_forwardPin, LOW);
    digitalWrite(_reversePin, LOW);
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

        // Send event to state machine based on completed operation
        if (_currentDirection == DIRECTION_EXTEND) {
            _stateMachine->processEvent(StateMachine::EVENT_DOOR_OPENED);
            Serial.println("ActuatorDriver: Door open operation complete, sending EVENT_DOOR_OPENED");
        } else if (_currentDirection == DIRECTION_RETRACT) {
            _stateMachine->processEvent(StateMachine::EVENT_DOOR_CLOSED);
            Serial.println("ActuatorDriver: Door close operation complete, sending EVENT_DOOR_CLOSED");
        }
    }
}

bool ActuatorDriver::isMoving() const {
    return _currentDirection != DIRECTION_STOP;
}
