// StepperDriver.cpp
#include "StepperDriver.h"

StepperDriver::StepperDriver(uint8_t stepPin, uint8_t dirPin, uint8_t enablePin)
    : _stepPin(stepPin), _dirPin(dirPin), _enablePin(enablePin),
      _speed(0), _clockwise(true), _enabled(false), _running(false),
      _lastStepTime(0), _stepInterval(0) {}

void StepperDriver::begin() {
    pinMode(_stepPin, OUTPUT);
    pinMode(_dirPin, OUTPUT);
    pinMode(_enablePin, OUTPUT);

    digitalWrite(_stepPin, LOW);
    digitalWrite(_dirPin, _clockwise ? HIGH : LOW);
    digitalWrite(_enablePin, HIGH);  // Most drivers use HIGH for disable

    _enabled = false;
    _running = false;
}

void StepperDriver::setSpeed(uint16_t stepsPerSecond) {
    _speed = stepsPerSecond;

    // Calculate step interval in microseconds
    if (_speed > 0) {
        _stepInterval = 1000000 / _speed;
    } else {
        _stepInterval = 0;
        _running = false;
    }
}

void StepperDriver::setDirection(bool clockwise) {
    _clockwise = clockwise;
    digitalWrite(_dirPin, _clockwise ? HIGH : LOW);
}

void StepperDriver::enable() {
    _enabled = true;
    digitalWrite(_enablePin, LOW);  // Most drivers use LOW for enable
}

void StepperDriver::disable() {
    _enabled = false;
    _running = false;
    digitalWrite(_enablePin, HIGH);  // Most drivers use HIGH for disable
}

void StepperDriver::startContinuous() {
    if (_speed > 0 && _enabled) {
        _running = true;
        _lastStepTime = micros();
    }
}

void StepperDriver::stop() {
    _running = false;
}

void StepperDriver::update() {
    if (!_running || !_enabled || _speed == 0) {
        return;
    }

    unsigned long currentMicros = micros();

    // Check if it's time for the next step
    if (currentMicros - _lastStepTime >= _stepInterval) {
        // Generate a step pulse
        digitalWrite(_stepPin, HIGH);
        delayMicroseconds(2);  // Minimum pulse width for most drivers
        digitalWrite(_stepPin, LOW);

        _lastStepTime = currentMicros;
    }
}

bool StepperDriver::isRunning() const {
    return _running && _enabled;
}
