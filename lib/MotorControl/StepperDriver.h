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

    // Speed control in RPM
    void setSpeed(uint16_t rpm);
    void setDirection(bool clockwise);

    void setSwingSpeed(uint8_t speedLevel); // Set swing speed (1=low, 2=medium, 3=high)

    // Enable/disable control
    void enable();
    void disable();

    // Movement control methods
    void startContinuous();     // Start continuous rotation
    void stop();               // Stop motion

    // Swing-specific methods
    void startSwinging();      // Begin 45° oscillating motion
    void stopSwinging();       // Stop swinging and return to center
    bool isSwinging() const;   // Check if currently swinging

    // Enhanced stop methods
    void emergencyHalt();      // Immediate stop, hold current position
    void smoothStop();         // Complete current swing, then stop at center
    void returnHome();         // Move to center position from anywhere

    // Position tracking
    bool isAtCenter() const;   // Check if at home position
    int getCurrentPosition() const;  // Position in steps from center
    float getCurrentAngle() const;   // Position in degrees from center

    // Must be called in loop() to handle stepping
    void update();

    // Status query
    bool isRunning() const;

private:
    Stepper _stepper;           // Arduino's Stepper library instance
    uint8_t _in1Pin, _in2Pin, _in3Pin, _in4Pin;  // L298N control pins
    int _stepsPerRevolution;    // Steps per full rotation
    uint16_t _speed;           // Speed in RPM
    bool _enabled;             // If motor is enabled
    bool _running;             // If motor is currently running
    bool _clockwise;           // Direction flag

    // Swing variables
    bool _swinging;            // If currently in swing mode
    bool _swingDirection;      // true = forward, false = backward
    int _swingSteps;           // Steps for 45 degrees

    // Position tracking
    int _currentPosition;      // Current position relative to center (in steps)
    bool _returningHome;       // Flag for home return operation
    bool _emergencyHalted;     // Flag for emergency stop state

    // Non-blocking swing physics
    unsigned long _lastStepTime;          // Last step timestamp
    unsigned long _swingStartTime;        // Swing start timestamp
    unsigned long _smoothStopStartTime;   // Smooth stop start timestamp
    uint16_t _stepInterval;               // Current step interval (ms)
    uint8_t _stepsPerInterval;            // Steps per interval (speed control)
    int _maxSwingSteps;                   // Maximum steps for swing angle
    float _currentSwingPhase;             // Current position in swing cycle (0.0-1.0)
    bool _smoothStopping;                 // Flag for smooth stop in progress

    // Physics calculation methods
    float calculateSwingProgress(unsigned long currentTime);
    int calculateTargetPosition(float progress);
    float calculateSinePosition(float progress);
    void updateSwingPhysics();

    // Helper methods
    void setPinsLow();         // Set all pins low (power saving)
    void updatePosition(int steps);  // Update position tracking
};
