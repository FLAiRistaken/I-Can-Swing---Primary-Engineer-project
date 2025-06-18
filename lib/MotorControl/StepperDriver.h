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
    void setSpeed(uint16_t rpm);

    // Direction control (maintained for compatibility)
    void setDirection(bool clockwise);

    // Enable/disable control
    void enable();
    void disable();

    // Movement control methods
    void startContinuous();  // Start continuous rotation
    void stop();             // Stop motion
    void step(int steps);    // Move specific number of steps

    // Must be called in loop() to handle stepping
    void update();

    // Status query
    bool isRunning() const;

    int getCurrentPosition() const;
    void resetPosition();

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

    int _currentPosition;

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
