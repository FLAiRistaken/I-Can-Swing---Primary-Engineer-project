// lib/MotorControl/StepperDriver.cpp
#include "StepperDriver.h"
#include "RuntimeConfig.h"
#include "Debug.h"

StepperDriver::StepperDriver(uint8_t in1Pin, uint8_t in2Pin, uint8_t in3Pin, uint8_t in4Pin, int stepsPerRev)
    : _stepper(stepsPerRev, in1Pin, in2Pin, in3Pin, in4Pin),
      _in1Pin(in1Pin),
      _in2Pin(in2Pin),
      _in3Pin(in3Pin),
      _in4Pin(in4Pin),
      _stepsPerRevolution(stepsPerRev),
      _speed(60),              // Default 60 RPM
      _enabled(false),
      _running(false),
      _clockwise(true),
      _swinging(false),
      _swingDirection(true),
      _swingSteps(0),
      _currentPosition(0),     // Start at center
      _returningHome(false),
      _emergencyHalted(false)
{
    // Constructor initialization complete
}

void StepperDriver::begin() {
    // Set up the control pins as outputs
    pinMode(_in1Pin, OUTPUT);
    pinMode(_in2Pin, OUTPUT);
    pinMode(_in3Pin, OUTPUT);
    pinMode(_in4Pin, OUTPUT);

    // Set all pins low initially (motor off)
    setPinsLow();

    // Configure the stepper speed
    _stepper.setSpeed(_speed);

    DEBUG_PRINT("StepperDriver: Initialized with pins ");
    DEBUG_PRINT(_in1Pin); DEBUG_PRINT(", ");
    DEBUG_PRINT(_in2Pin); DEBUG_PRINT(", ");
    DEBUG_PRINT(_in3Pin); DEBUG_PRINT(", ");
    DEBUG_PRINTLN(_in4Pin);
}

void StepperDriver::setSpeed(uint16_t rpm) {
    _speed = rpm;
    _stepper.setSpeed(_speed);
    DEBUG_PRINT("StepperDriver: Speed set to ");
    DEBUG_PRINT(_speed);
    DEBUG_PRINTLN(" RPM");
}

void StepperDriver::setDirection(bool clockwise) {
    _clockwise = clockwise;
}

void StepperDriver::enable() {
    _enabled = true;
    DEBUG_PRINTLN("StepperDriver: Enabled");
}

void StepperDriver::disable() {
    _enabled = false;
    _running = false;
    _swinging = false;
    _returningHome = false;
    _emergencyHalted = false;
    setPinsLow();
}

void StepperDriver::startContinuous() {
    if (_enabled && !_swinging && !_emergencyHalted) {
        _running = true;
        DEBUG_PRINT("StepperDriver: Started continuous rotation ");
        DEBUG_PRINTLN(_clockwise ? "clockwise" : "counter-clockwise");
    }
}

void StepperDriver::stop() {
    _running = false;
    _swinging = false;
    _returningHome = false;
    // Note: This is different from emergencyHalt - it allows normal operation to resume
}

void StepperDriver::startSwinging() {
    if (!_enabled) {
        Serial.println("StepperDriver: Cannot start swinging - motor disabled");
        return;
    }

    if (_emergencyHalted) {
        Serial.println("StepperDriver: Cannot start swinging - in emergency halt");
        return;
    }

    // Calculate steps for 45 degrees
    _swingSteps = (45 * _stepsPerRevolution) / 360;

    _swinging = true;
    _swingDirection = true;
    _running = false;
    _returningHome = false;

    // Move to initial +45° position
    int stepsToMove = _swingSteps - _currentPosition;
    _stepper.step(stepsToMove);
    updatePosition(stepsToMove);

    DEBUG_PRINTLN("StepperDriver: Started swinging motion");
}

void StepperDriver::stopSwinging() {
    if (_swinging) {
        _swinging = false;
        returnHome();  // Use the dedicated home return method
    }
}

// Enhanced stop methods
void StepperDriver::emergencyHalt() {
    _swinging = false;
    _running = false;
    _returningHome = false;
    _emergencyHalted = true;
    // Don't move - just stop immediately wherever we are
    Serial.println("StepperDriver: EMERGENCY HALT - holding current position");
}

void StepperDriver::smoothStop() {
    if (_swinging) {
        // Let current swing complete, then stop at center
        _swinging = false;
        _returningHome = true;
        Serial.println("StepperDriver: Smooth stop initiated - will return to center");
    } else if (_running) {
        // For continuous motion, just stop and return home
        _running = false;
        returnHome();
    }
}

void StepperDriver::returnHome() {
    if (_enabled && !_emergencyHalted) {
        _swinging = false;
        _running = false;
        _returningHome = true;
        DEBUG_PRINTLN("StepperDriver: Returning to home position");
    }
}

// Position tracking methods
bool StepperDriver::isAtCenter() const {
    return (_currentPosition == 0);
}

int StepperDriver::getCurrentPosition() const {
    return _currentPosition;
}

float StepperDriver::getCurrentAngle() const {
    return (_currentPosition * 360.0) / _stepsPerRevolution;
}

bool StepperDriver::isSwinging() const {
    return _swinging;
}

void StepperDriver::update() {
    if (!_enabled || _emergencyHalted) return;

    if (_returningHome) {
        // Move toward center position
        if (_currentPosition != 0) {
            int stepDirection = (_currentPosition > 0) ? -1 : 1;
            _stepper.step(stepDirection);
            updatePosition(stepDirection);

            if (_currentPosition == 0) {
                _returningHome = false;
                DEBUG_PRINTLN("StepperDriver: Reached home position");
            }
        } else {
            _returningHome = false;
        }
    } else if (_swinging) {
        // Enhanced swing logic with position tracking
        if (_swingDirection) {
            // Move from +45° to -45°
            int stepsToMove = -2 * _swingSteps;
            _stepper.step(stepsToMove);
            updatePosition(stepsToMove);
            _swingDirection = false;
        } else {
            // Move from -45° to +45°
            int stepsToMove = 2 * _swingSteps;
            _stepper.step(stepsToMove);
            updatePosition(stepsToMove);
            _swingDirection = true;
        }
    } else if (_running) {
        // Simple continuous rotation
        int stepDirection = _clockwise ? 1 : -1;
        _stepper.step(stepDirection);
        updatePosition(stepDirection);
    }
}

bool StepperDriver::isRunning() const {
    return (_running || _swinging || _returningHome) && _enabled && !_emergencyHalted;
}

void StepperDriver::setPinsLow() {
    digitalWrite(_in1Pin, LOW);
    digitalWrite(_in2Pin, LOW);
    digitalWrite(_in3Pin, LOW);
    digitalWrite(_in4Pin, LOW);
}

void StepperDriver::updatePosition(int steps) {
    _currentPosition += steps;
    // Optional: Add bounds checking or wraparound logic if needed
}
