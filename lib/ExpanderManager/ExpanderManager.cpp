#include "ExpanderManager.h"
#include "Debug.h"

ExpanderManager::ExpanderManager() : _pcf(0x20), _address(0x20),
                                   _lastFailTime(0), _consecutiveFailures(0) {
}

bool ExpanderManager::begin() {
    DEBUG_PRINTLN("ExpanderManager: Initializing PCF8575...");

    // PCF8575 initialization
    _pcf.begin();

    // Test if device responds
    if (!isConnected()) {
        Serial.println("ExpanderManager: ERROR - PCF8575 not found!");
        return false;
    }

    DEBUG_PRINTLN("ExpanderManager: PCF8575 found successfully.");
    initializePCF();
    return true;
}

void ExpanderManager::initializePCF() {
    // Configure all pins as inputs with pull-ups by default
    for (uint8_t pin = 0; pin < 16; pin++) {
        _pcf.pinMode(pin, INPUT);
    }
    DEBUG_PRINTLN("ExpanderManager: PCF8575 configured");
}

bool ExpanderManager::recoverI2C() {
    DEBUG_PRINTLN("ExpanderManager: I2C recovery attempt");

    // Simple I2C bus reset
    Wire.end();
    delay(100);
    Wire.begin();
    Wire.setClock(100000);  // 100kHz standard speed
    delay(100);

    // Reinitialize PCF8575
    _pcf.begin();

    // Test connection
    bool success = isConnected();
    if (success) {
        initializePCF();
        Serial.println("ExpanderManager: Recovery successful");
        _consecutiveFailures = 0;
    } else {
        Serial.println("ExpanderManager: Recovery failed");
    }

    return success;
}

bool ExpanderManager::checkI2CError() {
    Wire.beginTransmission(_address);
    int error = Wire.endTransmission();

    if (error != 0) {
        _consecutiveFailures++;
        _lastFailTime = millis();

        Serial.print("ExpanderManager: I2C error ");
        Serial.print(error);
        Serial.print(" (failure #");
        Serial.print(_consecutiveFailures);
        Serial.println(")");

        // Simple recovery for PCF8575 (much more reliable than MCP23017)
        if (_consecutiveFailures <= 3) {
            return recoverI2C();
        } else {
            Serial.println("ExpanderManager: Too many failures - PCF8575 may be damaged");
            return false;
        }
    }

    // Success - reset failure counter after 30 seconds
    if (_consecutiveFailures > 0 && (millis() - _lastFailTime > 30000)) {
        _consecutiveFailures = 0;
        Serial.println("ExpanderManager: Failure counter reset - system stable");
    }

    return true;
}

bool ExpanderManager::isConnected() {
    Wire.beginTransmission(_address);
    return (Wire.endTransmission() == 0);
}

void ExpanderManager::pinMode(uint8_t pin, uint8_t mode) {
    if (!checkI2CError()) {
        Serial.println("ExpanderManager: pinMode failed after recovery attempts");
        return;
    }
    _pcf.pinMode(pin, mode);
}

int ExpanderManager::digitalRead(uint8_t pin) {
    if (!checkI2CError()) {
        Serial.println("ExpanderManager: digitalRead failed after recovery attempts");
        return HIGH;  // Safe default
    }
    return _pcf.digitalRead(pin);
}

void ExpanderManager::digitalWrite(uint8_t pin, uint8_t state) {
    if (!checkI2CError()) {
        Serial.println("ExpanderManager: digitalWrite failed after recovery attempts");
        return;
    }
    _pcf.digitalWrite(pin, state);
}

int ExpanderManager::getConsecutiveFailures() const {
    return _consecutiveFailures;
}
