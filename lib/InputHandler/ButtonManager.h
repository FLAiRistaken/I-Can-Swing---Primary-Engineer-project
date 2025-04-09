// ButtonManager.h
#pragma once

#include <Arduino.h>
#include "Configuration.h"

class ButtonManager {
public:
    enum Button {
        BTN_START,
        BTN_STOP,
        BTN_SPEED_UP,
        BTN_SPEED_DOWN,
        BTN_DOOR,
        BTN_EMERGENCY,
        BTN_COUNT  // Use to define array sizes
    };

    ButtonManager();
    void begin();
    void update();

    bool isPressed(Button button);
    bool wasPressed(Button button);  // Returns true once per button press

    // Emergency stop handler
    static void emergencyStopISR();

private:
    uint8_t _pins[BTN_COUNT] = {
        PIN_BTN_START,
        PIN_BTN_STOP,
        PIN_BTN_SPEED_UP,
        PIN_BTN_SPEED_DOWN,
        PIN_BTN_DOOR,
        PIN_EMERGENCY_STOP
    };

    bool _currentState[BTN_COUNT] = {false};
    bool _lastState[BTN_COUNT] = {false};
    bool _pressedFlag[BTN_COUNT] = {false};
    unsigned long _lastDebounceTime[BTN_COUNT] = {0};

    static volatile bool _emergencyStop;
};
