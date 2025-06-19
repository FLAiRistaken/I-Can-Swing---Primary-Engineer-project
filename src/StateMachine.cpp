// StateMachine.cpp
#include "StateMachine.h"
#include "Debug.h"
#include "BuzzerDriver.h"
#include "ActuatorDriver.h"
#include "DoorActuatorManager.h"
#include "RuntimeConfig.h"
#include "StepperDriver.h"

StateMachine::StateMachine()
    : _currentState(STATE_IDLE),
      _currentSpeed(SPEED_OFF),
      _isUserPresent(false),
      _doorActuator(nullptr),
      _buzzer(nullptr),
      _swingMotors(nullptr),
      _stateEntryTime(0),
      _doorTimeoutMs(10000),
      _timeoutEnabled(false)
{}

void StateMachine::begin() {
    _currentState = STATE_IDLE;
    _currentSpeed = SPEED_OFF;
    _isUserPresent = false;
    _stateEntryTime = millis();
    _timeoutEnabled = false;
    _doorTimeoutMs = 10000; // 10 seconds default

    // Check if components are set
    if (!_buzzer) {
        Serial.println("StateMachine: Warning - Buzzer not set");
    }

    if (!_doorActuator) {
        Serial.println("StateMachine: Warning - Door actuator not set");
    }

    Serial.println("StateMachine: Initialized");
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
            resetFromEmergency();
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
                    // buzzer.beep(600, 150);
                }
            } else if (event == EVENT_DOOR_OPEN_PRESSED) {
                DEBUG_PRINTLN("StateMachine: Door toggle event in IDLE. Transitioning to DOOR_OPENING.");
                transition(STATE_DOOR_OPENING);
            } else if (event == EVENT_DOOR_CLOSE_PRESSED) {
                // In IDLE, door should be closed. If pressed, it could be a recalibration or error recovery.
                DEBUG_PRINTLN("StateMachine: Door CLOSE pressed in IDLE. Transitioning to DOOR_CLOSING (e.g., recalibrate/ensure closed).");
                transition(STATE_DOOR_CLOSING);
            }
            break;

        case STATE_SWINGING:
            DEBUG_PRINTLN("StateMachine: Handling event in SWINGING state.");
            if (event == EVENT_STOP_PRESSED) {
                DEBUG_PRINTLN("StateMachine: Stop pressed. Initiating smooth stop.");
                if (_swingMotors) {
                    _swingMotors->smoothStop();
                }
                _currentSpeed = SPEED_OFF;
                transition(STATE_IDLE);
            } else if (event == EVENT_SPEED_UP) {
                if (_currentSpeed < SPEED_HIGH) {
                    _currentSpeed = static_cast<Speed>(_currentSpeed + 1);
                    DEBUG_PRINT("StateMachine: Speed increased to "); DEBUG_PRINTLN(getSpeedString());
                    // Apply new speed to motor
                    if (_swingMotors) {
                        RuntimeConfig& config = RuntimeConfig::getInstance(); // Get instance
                        if (_currentSpeed == SPEED_LOW) _swingMotors->setSpeed(config.getSpeedLow());
                        else if (_currentSpeed == SPEED_MEDIUM) _swingMotors->setSpeed(config.getSpeedMedium());
                        else if (_currentSpeed == SPEED_HIGH) _swingMotors->setSpeed(config.getSpeedHigh());
                    }
                } else {
                    DEBUG_PRINTLN("StateMachine: Already at max speed.");
                }
            } else if (event == EVENT_SPEED_DOWN) {
                if (_currentSpeed > SPEED_LOW) {
                    _currentSpeed = static_cast<Speed>(_currentSpeed - 1);
                    DEBUG_PRINT("StateMachine: Speed decreased to "); DEBUG_PRINTLN(getSpeedString());
                    // Apply new speed to motor
                    if (_swingMotors) {
                        RuntimeConfig& config = RuntimeConfig::getInstance(); // Get instance
                        if (_currentSpeed == SPEED_LOW) _swingMotors->setSpeed(config.getSpeedLow());
                        else if (_currentSpeed == SPEED_MEDIUM) _swingMotors->setSpeed(config.getSpeedMedium());
                        else if (_currentSpeed == SPEED_HIGH) _swingMotors->setSpeed(config.getSpeedHigh());
                    }
                } else {
                    DEBUG_PRINTLN("StateMachine: Already at min speed.");
                }
            } else if (event == EVENT_SPEED_SET_LOW) { // ADD: Direct LOW speed button
                DEBUG_PRINTLN("StateMachine: Direct speed set to LOW.");
                _currentSpeed = SPEED_LOW;
                if (_swingMotors) {
                    RuntimeConfig& config = RuntimeConfig::getInstance(); // Get instance
                    _swingMotors->setSpeed(config.getSpeedLow());
                }
            } else if (event == EVENT_SPEED_SET_MEDIUM) { // ADD: Direct MEDIUM speed button
                DEBUG_PRINTLN("StateMachine: Direct speed set to MEDIUM.");
                _currentSpeed = SPEED_MEDIUM;
                if (_swingMotors) {
                    RuntimeConfig& config = RuntimeConfig::getInstance(); // Get instance
                    _swingMotors->setSpeed(config.getSpeedMedium());
                }
            } else if (event == EVENT_SPEED_SET_HIGH) { // ADD: Direct HIGH speed button
                DEBUG_PRINTLN("StateMachine: Direct speed set to HIGH.");
                _currentSpeed = SPEED_HIGH;
                if (_swingMotors) {
                    RuntimeConfig& config = RuntimeConfig::getInstance(); // Get instance
                    _swingMotors->setSpeed(config.getSpeedHigh());
                }
            } else if (event == EVENT_OBSTACLE_DETECTED) {
                DEBUG_PRINTLN("StateMachine: Obstacle detected while swinging. Transitioning to ERROR.");
                _currentSpeed = SPEED_OFF;
                transition(STATE_ERROR);
            } else if (event == EVENT_DOOR_OPEN_PRESSED || event == EVENT_DOOR_CLOSE_PRESSED) {
                DEBUG_PRINTLN("StateMachine: Door button pressed while swinging. Ignoring.");
            }
            break;

        case STATE_DOOR_OPENING:
            DEBUG_PRINTLN("StateMachine: Handling event in DOOR_OPENING state.");
            if (event == EVENT_DOOR_OPENED) { // Door opened fully (event from DoorActuatorManager)
                DEBUG_PRINTLN("StateMachine: Door opened fully. Transitioning to IDLE.");
                transition(STATE_IDLE);
            } else if (event == EVENT_DOOR_CLOSE_PRESSED) { // Allow interrupting opening to close
                DEBUG_PRINTLN("StateMachine: Door CLOSE pressed while opening. Transitioning to DOOR_CLOSING.");
                transition(STATE_DOOR_CLOSING);
            } else if (event == EVENT_DOOR_OPEN_PRESSED) { // Ignore if already trying to open
                DEBUG_PRINTLN("StateMachine: Door OPEN pressed while already opening. Ignoring.");
            } else if (event == EVENT_OBSTACLE_DETECTED) {
                DEBUG_PRINTLN("StateMachine: Obstacle detected while door opening. Transitioning to ERROR.");
                transition(STATE_ERROR);
            }
            break;

        case STATE_DOOR_CLOSING:
            DEBUG_PRINTLN("StateMachine: Handling event in DOOR_CLOSING state.");
            if (event == EVENT_DOOR_CLOSED) { // Door closed fully (event from DoorActuatorManager)
                DEBUG_PRINTLN("StateMachine: Door closed fully. Transitioning to IDLE.");
                transition(STATE_IDLE);
            } else if (event == EVENT_DOOR_OPEN_PRESSED) { // Allow interrupting closing to open
                DEBUG_PRINTLN("StateMachine: Door OPEN pressed while closing. Transitioning to DOOR_OPENING.");
                transition(STATE_DOOR_OPENING);
            } else if (event == EVENT_DOOR_CLOSE_PRESSED) { // Ignore if already trying to close
                DEBUG_PRINTLN("StateMachine: Door CLOSE pressed while already closing. Ignoring.");
            } else if (event == EVENT_OBSTACLE_DETECTED) {
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
    Serial.print("StateMachine: Entering state: ");
    Serial.println(getStateString());

    // Record time for timeout handling
    _stateEntryTime = millis();

    switch (state) {
        case STATE_SWINGING:
            Serial.print("StateMachine: Starting swing motion at speed: ");
            Serial.println(getSpeedString());
            if (_swingMotors) {
                RuntimeConfig& config = RuntimeConfig::getInstance();
                uint16_t speedValue = config.getSpeedLow(); // Always start at low
                _swingMotors->setSpeed(speedValue);
                _swingMotors->enable();
                _swingMotors->startSwinging();
            }
            // Audio feedback
            if (_buzzer) {
                _buzzer->beep(1000, 100);
                delay(50);
                _buzzer->beep(1200, 100);
            }
            break;

        case STATE_DOOR_OPENING:
            Serial.println("StateMachine: Initiating door opening sequence");
            // Direct control of door actuator
            if (_doorActuator) {
                _doorActuator->openDoor();
                _timeoutEnabled = true;
            }
            // Audio feedback
            if (_buzzer) {
                _buzzer->beep(800, 100);
            }
            break;

        case STATE_DOOR_CLOSING:
            Serial.println("StateMachine: Initiating door closing sequence");
            // Direct control of door actuator
            if (_doorActuator) {
                _doorActuator->closeDoor();
                _timeoutEnabled = true;
            }
            // Audio feedback
            if (_buzzer) {
                _buzzer->beep(600, 100);
            }
            break;

        case STATE_ERROR:
            Serial.println("StateMachine: Error condition detected");
            // Audio feedback - double beep
            if (_buzzer) {
                _buzzer->beep(1500, 200);
                delay(100);
                _buzzer->beep(1500, 200);
            }
            break;

        case STATE_EMERGENCY:
            Serial.println("StateMachine: EMERGENCY MODE ACTIVATED");

            if (_swingMotors) {
                _swingMotors->emergencyHalt();
            }
            // Audio feedback - urgent double beep
            if (_buzzer) {
                _buzzer->beep(2000, 500);
                delay(100);
                _buzzer->beep(2000, 500);
            }
            break;

        case STATE_IDLE:
            if (_swingMotors) {
                _swingMotors->returnHome();
                _swingMotors->disable();
            }
            Serial.println("StateMachine: System is now idle");
            // Audio feedback - single beep
            if (_buzzer) {
                _buzzer->beep(400, 100);
            }
            break;
    }
}

// StateMachine.cpp - Update exitState
void StateMachine::exitState(State state) {
    Serial.print("StateMachine: Exiting state: ");
    Serial.println(getStateString());

    switch (state) {
        case STATE_SWINGING:
            Serial.println("StateMachine: Exiting SWINGING state, ensuring motors are stopped.");
            if (_swingMotors) {
                _swingMotors->stopSwinging();
            }
            break;

        case STATE_DOOR_OPENING:
        case STATE_DOOR_CLOSING:
            // Stop door movement directly
            if (_doorActuator) {
                _doorActuator->stopDoor();
                _timeoutEnabled = false;
            }
            _timeoutEnabled = false;
            Serial.println("StateMachine: Stopping door movement");
            break;

        case STATE_ERROR:
        case STATE_EMERGENCY:
            Serial.println("StateMachine: Exiting fault state");
            // Audio feedback - recovery beep
            if (_buzzer) {
                _buzzer->beep(1000, 100);
                delay(50);
                _buzzer->beep(1500, 100);
            }
            break;
    }
}

void StateMachine::setDoorTimeout(unsigned long timeoutMs) {
    _doorTimeoutMs = timeoutMs;
    Serial.print("StateMachine: Door timeout set to ");
    Serial.print(_doorTimeoutMs);
    Serial.println(" ms");
}

void StateMachine::update() {
    // Check for timeouts
    if (_timeoutEnabled) {
        unsigned long currentTime = millis();
        if (currentTime - _stateEntryTime > _doorTimeoutMs) {
            Serial.println("StateMachine: Door operation timed out!");

            // Handle timeout based on current state
            if (_currentState == STATE_DOOR_OPENING || _currentState == STATE_DOOR_CLOSING) {
                if (_doorActuator) {
                    _doorActuator->stopDoor();
                }
                transition(STATE_ERROR);
            }

            _timeoutEnabled = false;
        }
    }
}

void StateMachine::resetFromEmergency() {
    if (_currentState == STATE_EMERGENCY && _swingMotors) {
        // Reset the emergency halt state in the motor
        _swingMotors->returnHome();  // Move to safe home position
        Serial.println("StateMachine: Emergency state reset, motor returning home");
    }
}


void StateMachine::setBuzzer(BuzzerDriver* buzzer) {
    _buzzer = buzzer;
}

void StateMachine::setDoorActuator(DoorActuatorManager* doorActuator) {
    _doorActuator = doorActuator;
}

void StateMachine::setSwingMotor(StepperDriver* motor) {
    _swingMotors = motor;
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
