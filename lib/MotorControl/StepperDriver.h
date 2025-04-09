// StepperDriver.h
#pragma once

#include <Arduino.h>
#include "Configuration.h"

class StepperDriver {
public:
    StepperDriver(uint8_t stepPin, uint8_t dirPin, uint8_t enablePin);

    void begin();
    void setSpeed(uint16_t stepsPerSecond);
    void setDirection(bool clockwise);
    void enable();
    void disable();

    // Non-blocking methods
    void startContinuous();
    void stop();

    // Must be called in loop() to handle stepping
    void update();

    bool isRunning() const;

private:
    uint8_t _stepPin;
    uint8_t _dirPin;
    uint8_t _enablePin;

    uint16_t _speed;        // Steps per second
    bool _clockwise;        // Direction
    bool _enabled;          // Motor enabled state
    bool _running;          // Whether motor should be stepping

    unsigned long _lastStepTime;
    unsigned long _stepInterval;  // Microseconds between steps
};
