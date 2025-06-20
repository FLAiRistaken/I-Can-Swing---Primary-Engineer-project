// ButtonManager.cpp
#include "ButtonManager.h"
#include "Configuration.h"
#include "ExpanderManager.h"

volatile bool ButtonManager::_emergencyStop = false;

ButtonManager::ButtonManager(ExpanderManager* expander) : _expander(expander) {}

void ButtonManager::begin() {
    Serial.println("ButtonManager: Initialising buttons...");

    // Initialise _pins array with corresponding pin definitions
    _pins[BTN_START] = PIN_BTN_START;
    _pins[BTN_STOP] = PIN_BTN_STOP;
    _pins[BTN_SPEED_LOW] = PIN_BTN_SPEED_LOW;
    _pins[BTN_SPEED_MEDIUM] = PIN_BTN_SPEED_MEDIUM;
    _pins[BTN_SPEED_HIGH] = PIN_BTN_SPEED_HIGH;
    _pins[BTN_DOOR_OPEN] = PIN_BTN_DOOR_OPEN;
    _pins[BTN_DOOR_CLOSE] = PIN_BTN_DOOR_CLOSE;
    _pins[BTN_ALERT] = PIN_BTN_ALERT;
    _pins[BTN_GIVE] = PIN_BTN_GIVE;
    _pins[BTN_EMERGENCY] = PIN_EMERGENCY_STOP;

    for (int i = 0; i <= BTN_GIVE; i++) {
        _expander->pinMode(_pins[i], INPUT_PULLUP);
    }
    Serial.println("ButtonManager: Expander pins configured.");
    // synchronises the software state with the physical hardware state.
    Serial.println("ButtonManager: Synchronising initial button states...");
    for (int i = 0; i <= BTN_GIVE; i++) {
        bool initialState = !_expander->digitalRead(_pins[i]); // Read the physical state
        _currentState[i] = initialState;
        _lastState[i] = initialState;
    }
    Serial.println("ButtonManager: Expander button states synchronised.");

    // Emergency stop pin setup
    pinMode(PIN_EMERGENCY_STOP, INPUT_PULLUP);
    Serial.println("ButtonManager: Emergency stop pin set as INPUT_PULLUP");
    // Attach interrupt pin
    attachInterrupt(digitalPinToInterrupt(PIN_EMERGENCY_STOP), emergencyStopISR, FALLING);
    Serial.println("ButtonManager: Emergency stop interrupt attached");
    Serial.println("ButtonManager: Initialisation complete");
}


void ButtonManager::update() {
    unsigned long currentTime = millis();

    for (int i = 0; i <= BTN_GIVE; i++) {

        // Read the current state (inverted because of INPUT_PULLUP)
        bool reading = !_expander->digitalRead(_pins[i]);

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
