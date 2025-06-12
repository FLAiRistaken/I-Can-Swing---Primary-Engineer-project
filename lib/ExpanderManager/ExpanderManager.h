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

private:
    Adafruit_MCP23X17 _mcp;
};