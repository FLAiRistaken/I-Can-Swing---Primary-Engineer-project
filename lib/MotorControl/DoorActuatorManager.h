#pragma once
#include "ActuatorDriver.h"
#include "ExpanderManager.h"
#include "StateMachine.h"

class DoorActuatorManager {
public:
    enum DoorState {
        DOOR_STOPPED,
        DOOR_OPENING,
        DOOR_CLOSING
    };

    // Constructor will take pins for both actuators and the shared expander/stateMachine
    DoorActuatorManager(uint8_t act1FwdPin, uint8_t act1RevPin,
                        uint8_t act2FwdPin, uint8_t act2RevPin,
                        StateMachine* stateMachine, ExpanderManager* expander);

    void begin();

    // Unified control methods
    void openDoor();
    void closeDoor();
    void stopDoor(); // Stops both

    // Management and feedback
    void update(); // Must be called in loop()
    bool isMoving() const;
    DoorState getDoorState() const;

private:
    ActuatorDriver _actuator1;
    ActuatorDriver _actuator2;
    StateMachine* _stateMachine;

    DoorState _currentDoorState;
    unsigned long _operationStartTime;
    unsigned long _operationTimeoutMs;
};
