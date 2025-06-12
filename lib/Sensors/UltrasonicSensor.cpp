// lib/Sensors/UltrasonicSensor.cpp
#include "UltrasonicSensor.h"

UltrasonicSensor::UltrasonicSensor(uint8_t trigPin, uint8_t echoPin, const char* name)
    : _trigPin(trigPin), _echoPin(echoPin), _name(name),
      _startTime(0), _echoPulseStartTime(0), _measuring(false), _lastDistance(500.00),
      _measurementAttempts(0), _lastAttemptTime(0) {}

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
    unsigned long duration = pulseIn(_echoPin, HIGH, 30000); // 30ms timeout

    // --- THIS IS THE FINAL FIX ---
    // If the duration is 0, it means the pulseIn() function timed out.
    // This is an invalid reading. Instead of returning 0.00, we return a safe, high value.
    if (duration == 0) {
        _lastDistance = 500.0f; // A safe, "no obstacle" value
        return _lastDistance;
    }
    // --- END OF FIX ---

    // If we have a valid duration, calculate the distance.
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
    _echoPulseStartTime = 0;
    _measuring = true;
    _measurementAttempts = 0;
}

bool UltrasonicSensor::isMeasurementComplete() {
    if (!_measuring) {
        return true; // No measurement in progress
    }

    // First, check for a total timeout. If it's been too long, give up.
    if (micros() - _startTime > MEASUREMENT_TIMEOUT) {
        _measuring = false;
        _lastDistance = 500.0; // Report the timeout value
        return true;
    }

    // If we have an active echo pulse, check its status
    if (_echoPulseStartTime > 0) {
        if (digitalRead(_echoPin) == LOW) {
            // SUCCESS: The pulse has just ended. Calculate the distance.
            unsigned long duration = micros() - _echoPulseStartTime;
            _lastDistance = (duration * 0.034) / 2.0;
            _measuring = false;
            return true; // Measurement is complete!
        }
        // else, pulse is still HIGH, so we continue waiting.
    }
    // If no pulse has started yet, check if the pin has gone HIGH
    else if (digitalRead(_echoPin) == HIGH) {
        // The echo pulse has just started. Record the time.
        _echoPulseStartTime = micros();
    }

    // If we've reached here, the measurement is not yet complete.
    return false;
}

float UltrasonicSensor::getLastDistance() {
    return _lastDistance;
}

const char* UltrasonicSensor::getName() const {
    return _name;
}
