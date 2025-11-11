// lib/Sensors/PressureSensor.cpp
#include "PressureSensor.h"
#include "Debug.h"

PressureSensor::PressureSensor(uint8_t analogPin, int threshold, const char* name)
    : _analogPin(analogPin), _threshold(threshold), _name(name), _lastRawValue(0) {}

void PressureSensor::begin() {
    pinMode(_analogPin, INPUT);
    DEBUG_PRINT("Pressure Sensor '");
    DEBUG_PRINT(_name);
    DEBUG_PRINTLN("' initialized");
}

int PressureSensor::readRawValue() {
    _lastRawValue = analogRead(_analogPin);
    return _lastRawValue;
}

bool PressureSensor::isOccupied() {
    readRawValue();
    //return (_lastRawValue > _threshold);
    return true;
}

int PressureSensor::getThreshold() const {
    return _threshold;
}

int PressureSensor::getLastReading() const {
    return _lastRawValue;
}

const char* PressureSensor::getName() const {
    return _name;
}
