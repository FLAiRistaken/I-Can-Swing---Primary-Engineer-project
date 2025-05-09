// StateMachine.cpp
#include "StateMachine.h"
#include "Debug.h"

StateMachine::StateMachine() : _currentState(STATE_IDLE), _currentSpeed(SPEED_OFF), _isUserPresent(false) {}

void StateMachine::begin() {
    // Initialise state machine
    _currentState = STATE_IDLE;
    _currentSpeed = SPEED_OFF;
    _isUserPresent = false;
    DEBUG_PRINTLN("StateMachine: Initialised. State: IDLE, UserPresent: false");
}

void StateMachine::processEvent(Event event) {
    DEBUG_PRINT("StateMachine: Received Event: "); DEBUG_PRINT(event); // Debugging
    DEBUG_PRINT(" in State: "); DEBUG_PRINTLN(getStateString());        // Debugging

    // --- Step 1: Highest Priority - Emergency Check ---
    if (event == EVENT_EMERGENCY) {
        // If already in EMERGENCY, do nothing further for this event.
        if (_currentState == STATE_EMERGENCY) {
             DEBUG_PRINTLN("StateMachine: Already in EMERGENCY state.");
             return;
        }
        DEBUG_PRINTLN("StateMachine: EMERGENCY event received! Transitioning to EMERGENCY state.");
        transition(STATE_EMERGENCY);
        return; // IMPORTANT: Stop processing immediately after handling emergency
    }

    // If we are currently in EMERGENCY state, ignore all non-emergency events
    // (We might add a specific "RESET" event later if needed)
    // If we are currently in EMERGENCY state, only respond to reset event
    if (_currentState == STATE_EMERGENCY) {
        if (event == EVENT_EMERGENCY_RESET) {
            DEBUG_PRINTLN("StateMachine: Emergency reset received. Transitioning to IDLE.");
            _currentSpeed = SPEED_OFF;
            transition(STATE_IDLE);
        } else {
            DEBUG_PRINTLN("StateMachine: In EMERGENCY state, ignoring non-reset event.");
        }
        return;
    }

    // --- Step 2: Update Persistent State Flags (like user presence) ---
    // These flags reflect the ongoing status based on sensor events.
    if (event == EVENT_PRESSURE_ON) {
        if (!_isUserPresent) { // Only log/act if state actually changes
             _isUserPresent = true;
             DEBUG_PRINTLN("StateMachine: User presence flag SET to true.");
             // Note: We don't transition state here, just update the flag.
             // The state logic below will use this updated flag.
        }
    } else if (event == EVENT_PRESSURE_OFF) {
        if (_isUserPresent) { // Only log/act if state actually changes
            _isUserPresent = false;
            DEBUG_PRINTLN("StateMachine: User presence flag CLEARED to false.");
            // Safety check: If user gets off while swinging, stop immediately.
            if (_currentState == STATE_SWINGING) {
                DEBUG_PRINTLN("StateMachine: User left while swinging. Transitioning to IDLE.");
                _currentSpeed = SPEED_OFF;
                transition(STATE_IDLE);
                // Since we transitioned, we are done processing this event for this cycle.
                return;
            }
        }
    }

    // --- Step 3: Handle State-Specific Logic ---
    // Now process the event based on the current state, using updated flags.
    DEBUG_PRINT("StateMachine: Processing event in state-specific logic. Current state: "); DEBUG_PRINTLN(getStateString());

    switch (_currentState) {
        case STATE_IDLE:
            DEBUG_PRINTLN("StateMachine: Handling event in IDLE state.");
            // Check START press AND if user is present flag is true
            if (event == EVENT_START_PRESSED) {
                if (_isUserPresent) { // Check the flag here
                    DEBUG_PRINTLN("StateMachine: Start pressed + User present. Transitioning to SWINGING.");
                    _currentSpeed = SPEED_LOW; // Start at low speed
                    transition(STATE_SWINGING);
                } else {
                    DEBUG_PRINTLN("StateMachine: Start pressed, but user not present. Doing nothing.");
                    // Optional: Add a short buzzer beep here to indicate why it didn't start
                    // buzzer.beep(600, 150);
                }
            } else if (event == EVENT_DOOR_TOGGLE) {
                DEBUG_PRINTLN("StateMachine: Door toggle event in IDLE. Transitioning to DOOR_OPENING.");
                transition(STATE_DOOR_OPENING);
            }
            // Note: EVENT_PRESSURE_ON/OFF are handled above by setting the flag.
            // No direct transition needed here based *only* on pressure change.
            break;

        case STATE_SWINGING:
            DEBUG_PRINTLN("StateMachine: Handling event in SWINGING state.");
            // Stop event OR pressure becoming OFF (handled in Step 2)
            if (event == EVENT_STOP_PRESSED) {
                 DEBUG_PRINTLN("StateMachine: Stop pressed. Transitioning to IDLE.");
                _currentSpeed = SPEED_OFF;
                transition(STATE_IDLE);
            } else if (event == EVENT_SPEED_UP) {
                if (_currentSpeed < SPEED_HIGH) {
                    _currentSpeed = static_cast<Speed>(_currentSpeed + 1);
                    DEBUG_PRINT("StateMachine: Speed increased to "); DEBUG_PRINTLN(getSpeedString());
                    // Need to signal motor controller about speed change here
                    // motorControl.setSpeed(_currentSpeed); // Example
                } else {
                     DEBUG_PRINTLN("StateMachine: Already at max speed.");
                }
            } else if (event == EVENT_SPEED_DOWN) {
                 if (_currentSpeed > SPEED_LOW) {
                    _currentSpeed = static_cast<Speed>(_currentSpeed - 1);
                    DEBUG_PRINT("StateMachine: Speed decreased to "); DEBUG_PRINTLN(getSpeedString());
                     // Need to signal motor controller about speed change here
                     // motorControl.setSpeed(_currentSpeed); // Example
                } else {
                     DEBUG_PRINTLN("StateMachine: Already at min speed.");
                }
            } else if (event == EVENT_OBSTACLE_DETECTED) {
                 DEBUG_PRINTLN("StateMachine: Obstacle detected while swinging. Transitioning to ERROR.");
                _currentSpeed = SPEED_OFF;
                transition(STATE_ERROR);
            }
            break;

        case STATE_DOOR_OPENING:
             DEBUG_PRINTLN("StateMachine: Handling event in DOOR_OPENING state.");
            if (event == EVENT_DOOR_OPENED) { // This event needs to be generated by the door actuator logic
                 DEBUG_PRINTLN("StateMachine: Door opened fully. Transitioning to IDLE.");
                transition(STATE_IDLE);
            } else if (event == EVENT_DOOR_TOGGLE) { // Allow interrupting opening to close
                 DEBUG_PRINTLN("StateMachine: Door toggle event while opening. Transitioning to DOOR_CLOSING.");
                transition(STATE_DOOR_CLOSING);
            } else if (event == EVENT_OBSTACLE_DETECTED) {
                // NEW: Add obstacle detection during door operations
                DEBUG_PRINTLN("StateMachine: Obstacle detected while door opening. Transitioning to ERROR.");
                transition(STATE_ERROR);
            }
            break;

        case STATE_DOOR_CLOSING:
             DEBUG_PRINTLN("StateMachine: Handling event in DOOR_CLOSING state.");
            if (event == EVENT_DOOR_CLOSED) { // This event needs to be generated by the door actuator logic
                 DEBUG_PRINTLN("StateMachine: Door closed fully. Transitioning to IDLE.");
                transition(STATE_IDLE);
            } else if (event == EVENT_DOOR_TOGGLE) { // Allow interrupting closing to open
                 DEBUG_PRINTLN("StateMachine: Door toggle event while closing. Transitioning to DOOR_OPENING.");
                transition(STATE_DOOR_OPENING);
            } else if (event == EVENT_OBSTACLE_DETECTED) {
                // NEW: Add obstacle detection during door operations
                DEBUG_PRINTLN("StateMachine: Obstacle detected while door closing. Transitioning to ERROR.");
                transition(STATE_ERROR);
            }
            break;

        case STATE_ERROR:
             DEBUG_PRINTLN("StateMachine: Handling event in ERROR state.");
            // Only allow clearing the error
            if (event == EVENT_ERROR_CLEARED) { // This event generated by button press in main.cpp
                 DEBUG_PRINTLN("StateMachine: Error cleared event. Transitioning to IDLE.");
                transition(STATE_IDLE);
            } else {
                DEBUG_PRINTLN("StateMachine: In ERROR state, ignoring other events.");
            }
            break;

        case STATE_EMERGENCY:
            // Should not reach here due to checks at the top, but included for completeness
            DEBUG_PRINTLN("StateMachine: In EMERGENCY state. No actions taken.");
            break;
    }
     DEBUG_PRINT("StateMachine: Event processing finished. Current state: "); DEBUG_PRINTLN(getStateString());
}

void StateMachine::transition(State newState) {
    exitState(_currentState);
    _currentState = newState;
    enterState(_currentState);
}

void StateMachine::enterState(State state) {
    DEBUG_PRINT("StateMachine: Entering state: ");
    DEBUG_PRINTLN(getStateString());

    // Actions to perform when entering each state
    switch (state) {
        case STATE_SWINGING:
            // Motors are controlled in updateMotors() in main.cpp
            // This just logs the entry
            DEBUG_PRINT("StateMachine: Starting swing motion at speed: ");
            DEBUG_PRINTLN(getSpeedString());
            break;

        case STATE_DOOR_OPENING:
            // Signal door to open via main loop
            DEBUG_PRINTLN("StateMachine: Initiating door opening sequence");
            // Main loop will handle the actual hardware control
            break;

        case STATE_DOOR_CLOSING:
            // Signal door to close via main loop
            DEBUG_PRINTLN("StateMachine: Initiating door closing sequence");
            // Main loop will handle the actual hardware control
            break;

        case STATE_ERROR:
            DEBUG_PRINTLN("StateMachine: Error condition detected");
            // Errors need to be cleared manually
            break;

        case STATE_EMERGENCY:
            DEBUG_PRINTLN("StateMachine: EMERGENCY MODE ACTIVATED");
            // Emergency needs manual reset
            break;

        case STATE_IDLE:
            DEBUG_PRINTLN("StateMachine: System is now idle");
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
