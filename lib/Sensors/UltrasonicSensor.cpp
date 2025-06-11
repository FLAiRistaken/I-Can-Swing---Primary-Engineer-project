// lib/Sensors/UltrasonicSensor.cpp
#include "UltrasonicSensor.h"

UltrasonicSensor::UltrasonicSensor(uint8_t trigPin, uint8_t echoPin, const char* name)
    : _trigPin(trigPin), _echoPin(echoPin), _name(name),
      _startTime(0), _echoPulseStartTime(0), _measuring(false), _lastDistance(500.00) {}

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

    // Calculate distance in cm
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
}

bool UltrasonicSensor::isMeasurementComplete() {
    if (!_measuring) {
        return true; // No measurement in progress
    }

    // First, check for a total timeout since the trigger pulse was sent.
    if (micros() - _startTime > MEASUREMENT_TIMEOUT) {
        _measuring = false;
        _lastDistance = 500.0; // Set to a safe, high value indicating no obstacle.
        return true;
    }

    // If the echo pin is HIGH, it means the pulse is traveling back to us.
    if (digitalRead(_echoPin) == HIGH) {
        // If this is the first time we see the HIGH signal, record the start time.
        if (_echoPulseStartTime == 0) {
            _echoPulseStartTime = micros();
        }
        // Measurement is not yet complete.
        return false;
    }
    // If the echo pin is LOW...
    else {
        // ...and we have previously recorded the start of the pulse, it means the pulse has just ended.
        if (_echoPulseStartTime > 0) {
            unsigned long duration = micros() - _echoPulseStartTime;
            _lastDistance = (duration * 0.034) / 2.0;
            _measuring = false;
            return true; // Measurement is complete!
        }
        // If we are here, it means the pin is LOW and we are still waiting for the pulse to start.
        // So, measurement is not yet complete.
        return false;
    }
}

float UltrasonicSensor::getLastDistance() {
    return _lastDistance;
}

const char* UltrasonicSensor::getName() const {
    return _name;
}
