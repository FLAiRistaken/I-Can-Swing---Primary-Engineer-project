#include "ExpanderManager.h"
#include "Debug.h"
#include <Wire.h>

ExpanderManager::ExpanderManager() : _address(0x20) {  // Default MCP23017 address
    // Constructor - store default I2C address
}

bool ExpanderManager::begin() {
    DEBUG_PRINTLN("ExpanderManager: Initializing MCP23017...");

    if (!_mcp.begin_I2C(_address)) {
        Serial.println("ExpanderManager: ERROR - MCP23017 not found!");
        return false;
    }

    DEBUG_PRINTLN("ExpanderManager: MCP23017 found successfully.");

    // Initialize MCP23017 configuration
    initializeMCP();

    return true;
}

void ExpanderManager::initializeMCP() {
    // Configure all pins as inputs with pull-ups by default
    // This matches your button configuration expectations
    for (uint8_t pin = 0; pin < 16; pin++) {
        _mcp.pinMode(pin, INPUT_PULLUP);
    }

    DEBUG_PRINTLN("ExpanderManager: MCP23017 registers initialized");
}

bool ExpanderManager::recoverI2C() {
    DEBUG_PRINTLN("ExpanderManager: Attempting I2C recovery...");

    // Reset I2C bus
    Wire.end();
    delay(100);
    Wire.begin();
    delay(100);

    // Test if MCP23017 responds
    Wire.beginTransmission(_address);
    int result = Wire.endTransmission();

    if (result == 0) {
        DEBUG_PRINTLN("ExpanderManager: I2C recovery successful");

        // Reinitialize MCP23017 registers
        if (_mcp.begin_I2C(_address)) {
            initializeMCP();
            return true;
        } else {
            Serial.println("ExpanderManager: Failed to reinitialize MCP23017 after recovery");
            return false;
        }
    } else {
        Serial.println("ExpanderManager: I2C recovery FAILED");
        return false;
    }
}

bool ExpanderManager::checkI2CError() {
    // Test I2C communication
    Wire.beginTransmission(_address);
    int error = Wire.endTransmission();

    if (error != 0) {
        Serial.print("ExpanderManager: I2C error detected (code ");
        Serial.print(error);
        Serial.println("), attempting recovery");

        return recoverI2C();
    }

    return true;  // No error
}

bool ExpanderManager::isConnected() {
    Wire.beginTransmission(_address);
    return (Wire.endTransmission() == 0);
}

void ExpanderManager::pinMode(uint8_t pin, uint8_t mode) {
    if (!checkI2CError()) {
        Serial.println("ExpanderManager: pinMode failed after I2C recovery attempt");
        return;
    }

    _mcp.pinMode(pin, mode);
}

int ExpanderManager::digitalRead(uint8_t pin) {
    if (!checkI2CError()) {
        Serial.println("ExpanderManager: digitalRead failed after I2C recovery attempt");
        return 0;  // Safe default for button reads (not pressed)
    }

    return _mcp.digitalRead(pin);
}

void ExpanderManager::digitalWrite(uint8_t pin, uint8_t state) {
    if (!checkI2CError()) {
        Serial.println("ExpanderManager: digitalWrite failed after I2C recovery attempt");
        return;
    }

    _mcp.digitalWrite(pin, state);
}
