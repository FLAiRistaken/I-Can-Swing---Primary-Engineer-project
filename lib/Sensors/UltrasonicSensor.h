// lib/Sensors/UltrasonicSensor.h
#pragma once

#include <Arduino.h>
#include "Configuration.h"

class UltrasonicSensor {
public:
    UltrasonicSensor(uint8_t trigPin, uint8_t echoPin, const char* name = "");
    void begin();

    // Measure distance (blocking method)
    float measureDistance();

    // Non-blocking measurement methods
    void startMeasurement();
    bool isMeasurementComplete();
    float getLastDistance();

    // Get sensor information
    const char* getName() const;

private:
    uint8_t _trigPin;
    uint8_t _echoPin;
    const char* _name;

    volatile unsigned long _startTime;
    volatile unsigned long _echoTime;
    volatile bool _measuring;
    float _lastDistance;

    // Timeout constants
    static const unsigned long MEASUREMENT_TIMEOUT = 38000; // 38ms timeout (max HC-SR04 range)
};
