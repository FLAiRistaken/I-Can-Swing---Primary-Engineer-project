// ButtonManager.cpp
#include "ButtonManager.h"
#include "Configuration.h"

volatile bool ButtonManager::_emergencyStop = false;

ButtonManager::ButtonManager() {}

void ButtonManager::begin() {
    Serial.println("ButtonManager: Initializing buttons...");
    for (int i = 0; i < BTN_COUNT; i++) {
        pinMode(_pins[i], INPUT_PULLUP);
        Serial.print("ButtonManager: Pin ");
        Serial.print(_pins[i]);
        Serial.println(" set as INPUT_PULLUP");
    }
    // synchronises the software state with the physical hardware state.
    Serial.println("ButtonManager: Synchronizing initial button states...");
    for (int i = 0; i < BTN_COUNT; i++) {
        bool initialState = !digitalRead(_pins[i]); // Read the physical state
        _currentState[i] = initialState;
        _lastState[i] = initialState;
    }
    // Emergency stop pin setup
    pinMode(PIN_EMERGENCY_STOP, INPUT_PULLUP);
    Serial.println("ButtonManager: Emergency stop pin set as INPUT_PULLUP");
    // Attach interrupt pin
    attachInterrupt(digitalPinToInterrupt(PIN_EMERGENCY_STOP), emergencyStopISR, FALLING);
    Serial.println("ButtonManager: Emergency stop interrupt attached");
    Serial.println("ButtonManager: Initialization complete");
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
        if (_emergencyStop) {
            _emergencyStop = false;
            return true;
        }
        return false;
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
