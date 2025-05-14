// lib/MotorControl/StepperDriver.h
#pragma once

#include <Arduino.h>
#include <Stepper.h>
#include "Configuration.h"

class StepperDriver {
public:
    // Constructor for L298N control (4 pins)
    StepperDriver(uint8_t in1Pin, uint8_t in2Pin, uint8_t in3Pin, uint8_t in4Pin, int stepsPerRev = 200);

    void begin();

    // Speed control in RPM (not steps per second like before)
    virtual void setSpeed(uint16_t rpm);

    virtual uint16_t getSpeed();

    // Direction control (maintained for compatibility)
    void setDirection(bool clockwise);

    // Enable/disable control
    virtual void enable();
    virtual void disable();

    // Movement control methods
    virtual void startContinuous();  // Start continuous rotation
    virtual void stop();             // Stop motion
    virtual void step(int steps);    // Move specific number of steps

    // Must be called in loop() to handle stepping
    virtual void update();

    // Status query
    virtual bool isRunning() const;

private:
    Stepper _stepper;         // Arduino's Stepper library instance
    uint8_t _in1Pin;          // L298N control pin 1
    uint8_t _in2Pin;          // L298N control pin 2
    uint8_t _in3Pin;          // L298N control pin 3
    uint8_t _in4Pin;          // L298N control pin 4

    int _stepsPerRevolution;  // Steps per full rotation (typically 200)
    uint16_t _speed;          // Speed in RPM
    bool _enabled;            // If motor is enabled
    bool _running;            // If motor is currently running
    bool _clockwise;          // Direction flag

    // For non-blocking operation
    int _targetSteps;         // Target step count for non-blocking moves
    int _currentSteps;        // Current step count
    unsigned long _lastStepTime;  // Timing control
    unsigned long _stepInterval;  // Time between steps (microseconds)

    // Convert between RPM and step interval
    void calculateStepInterval();

    // Helper to set all pins low (power saving)
    void setPinsLow();
};
