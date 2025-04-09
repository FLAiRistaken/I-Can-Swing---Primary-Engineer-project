// StateMachine.cpp
#include "StateMachine.h"

StateMachine::StateMachine() : _currentState(STATE_IDLE), _currentSpeed(SPEED_OFF) {}

void StateMachine::begin() {
    // Initialize state machine
    _currentState = STATE_IDLE;
    _currentSpeed = SPEED_OFF;
}

void StateMachine::processEvent(Event event) {
    // Handle emergency events in any state
    if (event == EVENT_EMERGENCY) {
        transition(STATE_EMERGENCY);
        return;
    }

    // State-specific event handling
    switch (_currentState) {
        case STATE_IDLE:
            if (event == EVENT_START_PRESSED && event == EVENT_PRESSURE_ON) {
                _currentSpeed = SPEED_LOW;
                transition(STATE_SWINGING);
            } else if (event == EVENT_DOOR_TOGGLE) {
                transition(STATE_DOOR_OPENING);
            }
            break;

        case STATE_SWINGING:
            if (event == EVENT_STOP_PRESSED || event == EVENT_PRESSURE_OFF) {
                _currentSpeed = SPEED_OFF;
                transition(STATE_IDLE);
            } else if (event == EVENT_SPEED_UP) {
                if (_currentSpeed < SPEED_HIGH) {
                    _currentSpeed = static_cast<Speed>(_currentSpeed + 1);
                }
            } else if (event == EVENT_SPEED_DOWN) {
                if (_currentSpeed > SPEED_LOW) {
                    _currentSpeed = static_cast<Speed>(_currentSpeed - 1);
                }
            } else if (event == EVENT_OBSTACLE_DETECTED) {
                _currentSpeed = SPEED_OFF;
                transition(STATE_ERROR);
            }
            break;

        case STATE_DOOR_OPENING:
            if (event == EVENT_DOOR_OPENED) {
                transition(STATE_IDLE);
            } else if (event == EVENT_DOOR_TOGGLE) {
                transition(STATE_DOOR_CLOSING);
            }
            break;

        case STATE_DOOR_CLOSING:
            if (event == EVENT_DOOR_CLOSED) {
                transition(STATE_IDLE);
            } else if (event == EVENT_DOOR_TOGGLE) {
                transition(STATE_DOOR_OPENING);
            }
            break;

        case STATE_ERROR:
            if (event == EVENT_ERROR_CLEARED) {
                transition(STATE_IDLE);
            }
            break;

        case STATE_EMERGENCY:
            // Only manual reset can exit emergency state
            break;
    }
}

void StateMachine::transition(State newState) {
    exitState(_currentState);
    _currentState = newState;
    enterState(_currentState);
}

void StateMachine::enterState(State state) {
    // Actions to perform when entering each state
    switch (state) {
        case STATE_SWINGING:
            // Start motors at current speed
            break;

        case STATE_DOOR_OPENING:
            // Start actuator
            break;

        case STATE_DOOR_CLOSING:
            // Start actuator in reverse
            break;

        case STATE_EMERGENCY:
            // Stop all motors immediately
            _currentSpeed = SPEED_OFF;
            break;

        default:
            break;
    }
}

void StateMachine::exitState(State state) {
    // Actions to perform when exiting each state
    switch (state) {
        case STATE_SWINGING:
            // Stop swing motors
            break;

        case STATE_DOOR_OPENING:
        case STATE_DOOR_CLOSING:
            // Stop actuator
            break;

        default:
            break;
    }
}

StateMachine::State StateMachine::getCurrentState() const {
    return _currentState;
}

StateMachine::Speed StateMachine::getCurrentSpeed() const {
    return _currentSpeed;
}

const char* StateMachine::getStateString() const {
    switch (_currentState) {
        case STATE_IDLE: return "IDLE";
        case STATE_SWINGING: return "SWINGING";
        case STATE_DOOR_OPENING: return "DOOR OPENING";
        case STATE_DOOR_CLOSING: return "DOOR CLOSING";
        case STATE_ERROR: return "ERROR";
        case STATE_EMERGENCY: return "EMERGENCY STOP";
        default: return "UNKNOWN";
    }
}

const char* StateMachine::getSpeedString() const {
    switch (_currentSpeed) {
        case SPEED_OFF: return "OFF";
        case SPEED_LOW: return "LOW";
        case SPEED_MEDIUM: return "MEDIUM";
        case SPEED_HIGH: return "HIGH";
        default: return "UNKNOWN";
    }
}
