// lib/Sensors/PressureSensor.h
#pragma once

#include <Arduino.h>
#include "Configuration.h"

class PressureSensor {
public:
    PressureSensor(uint8_t analogPin, int threshold, const char* name = "");
    void begin();

    // Reads the raw analog value
    int readRawValue();

    // Checks if the pressure threshold is exceeded
    bool isOccupied();

    // Get threshold and last reading
    int getThreshold() const;
    int getLastReading() const;

    // Get sensor name
    const char* getName() const;

private:
    uint8_t _analogPin;
    int _threshold;
    const char* _name;
    int _lastRawValue;
};
