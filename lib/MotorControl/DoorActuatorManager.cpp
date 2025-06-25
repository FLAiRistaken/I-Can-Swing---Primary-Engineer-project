#include "DoorActuatorManager.h"
#include "RuntimeConfig.h"
#include "Debug.h"

// Constructor initializes both internal ActuatorDriver instances
DoorActuatorManager::DoorActuatorManager(uint8_t act1FwdPin, uint8_t act1RevPin,
                                          uint8_t act2FwdPin, uint8_t act2RevPin,
                                          StateMachine* stateMachine, ExpanderManager* expander)
    : _actuator1(act1FwdPin, act1RevPin, expander),
      _actuator2(act2FwdPin, act2RevPin, expander),
      _stateMachine(stateMachine),
      _currentDoorState(DOOR_STOPPED),
      _operationStartTime(0),
      _operationTimeoutMs(16000) // Default timeout for full travel
{}

void DoorActuatorManager::begin() {
    _actuator1.begin();
    _actuator2.begin();
    DEBUG_PRINTLN("DoorActuatorManager: Initialized both actuators.");
}

void DoorActuatorManager::openDoor() {
    if (_currentDoorState != DOOR_STOPPED) {
        DEBUG_PRINTLN("DoorActuatorManager: Door already in motion.");
        return;
    }
    DEBUG_PRINTLN("DoorActuatorManager: Opening door...");
    _actuator1.startExtend(_operationTimeoutMs); // Start timed operation
    _actuator2.startExtend(_operationTimeoutMs); // Start timed operation
    _currentDoorState = DOOR_OPENING;
    _operationStartTime = millis();
}

void DoorActuatorManager::closeDoor() {
    if (_currentDoorState != DOOR_STOPPED) {
        DEBUG_PRINTLN("DoorActuatorManager: Door already in motion.");
        return;
    }
    DEBUG_PRINTLN("DoorActuatorManager: Closing door...");
    _actuator1.startRetract(_operationTimeoutMs); // Start timed operation
    _actuator2.startRetract(_operationTimeoutMs); // Start timed operation
    _currentDoorState = DOOR_CLOSING;
    _operationStartTime = millis();
}

void DoorActuatorManager::stopDoor() {
    DEBUG_PRINTLN("DoorActuatorManager: Stopping door motion.");
    _actuator1.stop();
    _actuator2.stop();
    _currentDoorState = DOOR_STOPPED;
    _operationStartTime = 0;
}

void DoorActuatorManager::update() {
    _actuator1.update(); // Update internal actuator states
    _actuator2.update();

    if (_currentDoorState == DOOR_OPENING || _currentDoorState == DOOR_CLOSING) {
        // Check if both actuators have completed their timed operation
        if (!_actuator1.isMoving() && !_actuator2.isMoving()) {
            DEBUG_PRINTLN("DoorActuatorManager: Both actuators stopped.");
            DoorState finishedState = _currentDoorState; // Capture before stopping
            _currentDoorState = DOOR_STOPPED; // Set manager's state to stopped

            // Signal StateMachine about completion
            if (_stateMachine) { // Ensure StateMachine pointer is valid
                if (finishedState == DOOR_OPENING) {
                    _stateMachine->processEvent(StateMachine::EVENT_DOOR_OPENED);
                    Serial.println("DoorActuatorManager: Sent EVENT_DOOR_OPENED to StateMachine.");
                } else if (finishedState == DOOR_CLOSING) {
                    _stateMachine->processEvent(StateMachine::EVENT_DOOR_CLOSED);
                    Serial.println("DoorActuatorManager: Sent EVENT_DOOR_CLOSED to StateMachine.");
                }
            } else {
                Serial.println("DoorActuatorManager: StateMachine pointer is null, cannot send event.");
            }
        } else if (millis() - _operationStartTime > _operationTimeoutMs + 500) {
            // A bit of buffer time after official timeout, just in case one is lagging
            Serial.println("DoorActuatorManager: Operation timeout triggered (manager level).");
            stopDoor(); // Force stop if still moving way past timeout

            // Signal StateMachine about timeout/error if it's not already stopped
            if (_stateMachine && (_currentDoorState == DOOR_OPENING || _currentDoorState == DOOR_CLOSING)) {
                // Consider a specific error event for timeouts, e.g., EVENT_DOOR_TIMEOUT
                _stateMachine->processEvent(StateMachine::EVENT_OBSTACLE_DETECTED); // Using a generic error event for now
                Serial.println("DoorActuatorManager: Sent EVENT_OBSTACLE_DETECTED due to timeout.");
            }
        }
    }
}

bool DoorActuatorManager::isMoving() const {
    return _actuator1.isMoving() || _actuator2.isMoving();
}

DoorActuatorManager::DoorState DoorActuatorManager::getDoorState() const {
    return _currentDoorState;
}
