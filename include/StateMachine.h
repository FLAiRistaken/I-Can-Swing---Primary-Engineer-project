// StateMachine.h
#pragma once

#include <Arduino.h>
#include "Configuration.h"

class StateMachine {
public:
    enum State {
        STATE_IDLE,
        STATE_SWINGING,
        STATE_DOOR_OPENING,
        STATE_DOOR_CLOSING,
        STATE_ERROR,
        STATE_EMERGENCY
    };

    enum Event {
        EVENT_NONE,
        EVENT_START_PRESSED,
        EVENT_STOP_PRESSED,
        EVENT_SPEED_UP,
        EVENT_SPEED_DOWN,
        EVENT_DOOR_TOGGLE,
        EVENT_PRESSURE_ON,
        EVENT_PRESSURE_OFF,
        EVENT_OBSTACLE_DETECTED,
        EVENT_DOOR_OPENED,
        EVENT_DOOR_CLOSED,
        EVENT_EMERGENCY,
        EVENT_ERROR_CLEARED
    };

    enum Speed {
        SPEED_OFF,
        SPEED_LOW,
        SPEED_MEDIUM,
        SPEED_HIGH
    };

    StateMachine();
    void begin();

    // Process an event and update state
    void processEvent(Event event);

    // Get current state
    State getCurrentState() const;

    // Get current speed
    Speed getCurrentSpeed() const;

    // Get string descriptions of states for display
    const char* getStateString() const;
    const char* getSpeedString() const;

private:
    State _currentState;
    Speed _currentSpeed;

    // Transition to a new state
    void transition(State newState);

    // State entry actions
    void enterState(State state);

    // State exit actions
    void exitState(State state);
};
