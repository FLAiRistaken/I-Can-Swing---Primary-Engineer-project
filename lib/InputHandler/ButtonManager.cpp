// ButtonManager.cpp
#include "ButtonManager.h"

volatile bool ButtonManager::_emergencyStop = false;

ButtonManager::ButtonManager() {}

void ButtonManager::begin() {
    for (int i = 0; i < BTN_COUNT; i++) {
        pinMode(_pins[i], INPUT_PULLUP);
    }

    // Set up emergency stop interrupt
    attachInterrupt(digitalPinToInterrupt(PIN_EMERGENCY_STOP), emergencyStopISR, FALLING);
}

void ButtonManager::update() {
    unsigned long currentTime = millis();

    for (int i = 0; i < BTN_COUNT; i++) {
        // Skip emergency button as it's handled by interrupt
        if (i == BTN_EMERGENCY) continue;

        // Read the current state (inverted because of INPUT_PULLUP)
        bool reading = !digitalRead(_pins[i]);

        // Check if button state changed
        if (reading != _lastState[i]) {
            _lastDebounceTime[i] = currentTime;
        }

        // If enough time passed since last change, update the state
        if ((currentTime - _lastDebounceTime[i]) > BUTTON_DEBOUNCE_MS) {
            if (reading != _currentState[i]) {
                _currentState[i] = reading;

                // Set the flag when button is pressed
                if (_currentState[i]) {
                    _pressedFlag[i] = true;
                }
            }
        }

        _lastState[i] = reading;
    }
}

bool ButtonManager::isPressed(Button button) {
    if (button == BTN_EMERGENCY) {
        return _emergencyStop;
    }
    return _currentState[button];
}

bool ButtonManager::wasPressed(Button button) {
    if (button == BTN_EMERGENCY) {
        bool temp = _emergencyStop;
        _emergencyStop = false;
        return temp;
    }

    if (_pressedFlag[button]) {
        _pressedFlag[button] = false;
        return true;
    }
    return false;
}

void ButtonManager::emergencyStopISR() {
    _emergencyStop = true;
}
