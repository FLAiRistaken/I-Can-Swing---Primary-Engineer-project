// lib/MotorControl/ActuatorDriver.h
#pragma once

#include <Arduino.h>
#include "Configuration.h"

class StateMachine;
class ExpanderManager;

class ActuatorDriver {
public:
    enum Direction {
        DIRECTION_STOP,
        DIRECTION_EXTEND,
        DIRECTION_RETRACT
    };

    ActuatorDriver(uint8_t forwardPin, uint8_t reversePin, StateMachine* stateMachine, ExpanderManager* expander);
    void begin();

    // Control methods
    void extend();  // Open door
    void retract(); // Close door
    void stop();

    // Current status
    Direction getCurrentDirection() const;

    // Non-blocking control with timing
    void startExtend(unsigned long timeMs = DOOR_OPEN_TIME_MS);
    void startRetract(unsigned long timeMs = DOOR_OPEN_TIME_MS);
    void update(); // Call this in loop()
    bool isMoving() const;

private:
    uint8_t _forwardPin;
    uint8_t _reversePin;
    Direction _currentDirection;
    StateMachine* _stateMachine;  // Reference to state machine for events
    ExpanderManager* _expander;

    // For timed operation
    unsigned long _startTime;
    unsigned long _operationDuration;
    bool _timedOperation;
};
