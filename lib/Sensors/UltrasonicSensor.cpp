// lib/Sensors/UltrasonicSensor.cpp
#include "UltrasonicSensor.h"

UltrasonicSensor::UltrasonicSensor(uint8_t trigPin, uint8_t echoPin, const char* name)
    : _trigPin(trigPin), _echoPin(echoPin), _name(name),
      _startTime(0), _echoTime(0), _measuring(false), _lastDistance(0.0) {}

void UltrasonicSensor::begin() {
    pinMode(_trigPin, OUTPUT);
    pinMode(_echoPin, INPUT);
    digitalWrite(_trigPin, LOW);
}

float UltrasonicSensor::measureDistance() {
    // Clear trigger pin
    digitalWrite(_trigPin, LOW);
    delayMicroseconds(2);

    // Set trigger pin HIGH for 10 microseconds
    digitalWrite(_trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(_trigPin, LOW);

    // Read echo pin (time in microseconds)
    unsigned long duration = pulseIn(_echoPin, HIGH, MEASUREMENT_TIMEOUT);

    // Calculate distance in cm (speed of sound = 0.034 cm/µs)
    // Divide by 2 because sound travels to the object and back
    _lastDistance = (duration * 0.034) / 2.0;

    return _lastDistance;
}

void UltrasonicSensor::startMeasurement() {
    if (_measuring) {
        return; // Already measuring
    }

    // Clear trigger pin
    digitalWrite(_trigPin, LOW);
    delayMicroseconds(2);

    // Set trigger pin HIGH for 10 microseconds
    digitalWrite(_trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(_trigPin, LOW);

    _startTime = micros();
    _measuring = true;
}

bool UltrasonicSensor::isMeasurementComplete() {
    if (!_measuring) {
        return true; // No measurement in progress
    }

    // Check if echo pin is HIGH
    if (digitalRead(_echoPin) == HIGH) {
        // Still waiting for echo
        if (micros() - _startTime > MEASUREMENT_TIMEOUT) {
            // Timeout - no object detected
            _measuring = false;
            _lastDistance = -1.0; // Invalid distance
            return true;
        }
        return false; // Still measuring
    } else {
        // Echo received or never started
        if (_echoTime == 0) {
            // Echo never started or already ended
            _measuring = false;
            return true;
        }

        // Calculate duration and distance
        unsigned long duration = _echoTime - _startTime;
        _lastDistance = (duration * 0.034) / 2.0;
        _measuring = false;
        return true;
    }
}

float UltrasonicSensor::getLastDistance() {
    return _lastDistance;
}

const char* UltrasonicSensor::getName() const {
    return _name;
}
