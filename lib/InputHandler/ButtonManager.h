// ButtonManager.h
#pragma once

#include <Arduino.h>
#include "Configuration.h"
#include "ExpanderManager.h"

class ButtonManager {
public:
    enum Button {
        BTN_STOP,
        BTN_SPEED_LOW,
        BTN_SPEED_MEDIUM,
        BTN_SPEED_HIGH,
        BTN_DOOR_OPEN,
        BTN_DOOR_CLOSE,
        BTN_ALERT,
        BTN_GIVE,
        BTN_EMERGENCY,
        BTN_COUNT  // Use to define array sizes
    };

    ButtonManager(ExpanderManager* expander);
    void begin();
    void update();

    bool isPressed(Button button);
    bool wasPressed(Button button);  // Returns true once per button press

    // Emergency stop handler
    static void emergencyStopISR();

private:
    ExpanderManager* _expander;

    uint8_t _pins[BTN_COUNT];

    bool _currentState[BTN_COUNT] = {false};
    bool _lastState[BTN_COUNT] = {false};
    bool _pressedFlag[BTN_COUNT] = {false};
    unsigned long _lastDebounceTime[BTN_COUNT] = {0};

    static volatile bool _emergencyStop;
};
