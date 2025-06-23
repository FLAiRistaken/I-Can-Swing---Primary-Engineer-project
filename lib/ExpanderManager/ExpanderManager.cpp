#include "ExpanderManager.h"
#include "Debug.h"
#include <Arduino.h>

ExpanderManager::ExpanderManager() {
    // Constructor is empty
}

bool ExpanderManager::begin() {
    DEBUG_PRINTLN("ExpanderManager: Initializing MCP23017...");
    if (!_mcp.begin_I2C()) {
        Serial.println("ExpanderManager: ERROR - MCP23017 not found!");
        return false;
    }
    DEBUG_PRINTLN("ExpanderManager: MCP23017 found successfully.");
    return true;
}

// This single function handles setting the mode, including the pull-up.
void ExpanderManager::pinMode(uint8_t pin, uint8_t mode) {
    _mcp.pinMode(pin, mode);
}

int ExpanderManager::digitalRead(uint8_t pin) {
    return _mcp.digitalRead(pin);
}

void ExpanderManager::digitalWrite(uint8_t pin, uint8_t state) {
    _mcp.digitalWrite(pin, state);
}
