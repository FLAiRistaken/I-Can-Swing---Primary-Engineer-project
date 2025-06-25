#pragma once

#include <Adafruit_MCP23X17.h>

class ExpanderManager {
public:
    ExpanderManager();
    bool begin();
    void pinMode(uint8_t pin, uint8_t mode);
    void pullUp(uint8_t pin, uint8_t state);
    int digitalRead(uint8_t pin);
    void digitalWrite(uint8_t pin, uint8_t state);

    bool recoverI2C();
    bool isConnected();

private:
    Adafruit_MCP23X17 _mcp;

    uint8_t _address;  // Store I2C address for recovery

    // Initialize MCP23017 registers
    void initializeMCP();

    // Check for I2C errors and attempt recovery
    bool checkI2CError();
};