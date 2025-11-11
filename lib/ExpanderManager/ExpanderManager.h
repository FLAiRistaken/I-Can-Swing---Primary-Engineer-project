#pragma once
#include <Wire.h>
#include "PCF8575.h"  

class ExpanderManager {
public:
    ExpanderManager();
    bool begin();
    void pinMode(uint8_t pin, uint8_t mode);
    int digitalRead(uint8_t pin);
    void digitalWrite(uint8_t pin, uint8_t state);
    bool recoverI2C();
    bool isConnected();

    // Recovery methods
    int getConsecutiveFailures() const;

private:
    PCF8575 _pcf;
    uint8_t _address;
    unsigned long _lastFailTime;
    int _consecutiveFailures;

    void initializePCF();
    bool checkI2CError();
};
