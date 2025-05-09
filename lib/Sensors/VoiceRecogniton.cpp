// lib/Sensors/VoiceRecognition.cpp
#include "VoiceRecognition.h"

VoiceRecognition::VoiceRecognition(uint8_t rxPin, uint8_t txPin, StateMachine* stateMachine)
    : myVR(rxPin, txPin), _stateMachine(stateMachine) {
    // Initialize records array
    for (int i = 0; i < CMD_COUNT; i++) {
        records[i] = i;  // Record ID matches command enum
    }
}

void VoiceRecognition::begin() {
    // Initialize VR module
    myVR.begin(9600);

    Serial.println("VoiceRecognition: Initializing...");

    // Load voice commands
    if (myVR.load(records, CMD_COUNT)) {
        Serial.println("VoiceRecognition: Voice commands loaded successfully");
    } else {
        Serial.println("VoiceRecognition: Error loading voice commands");
    }

    Serial.println("VoiceRecognition: Ready");
}

void VoiceRecognition::update() {
    // Check if voice command received
    int ret = myVR.recognize(buf, 50);

    if (ret > 0) {
        // Command recognized
        int commandId = buf[1]; // Get command ID
        Serial.print("VoiceRecognition: Command recognized: ");
        Serial.println(commandId);

        handleVoiceCommand(commandId);
    }
}

void VoiceRecognition::handleVoiceCommand(int command) {
    // Map voice commands to state machine events
    switch(command) {
        case CMD_START:
            Serial.println("VoiceRecognition: START command received");
            _stateMachine->processEvent(StateMachine::EVENT_START_PRESSED);
            break;

        case CMD_STOP:
            Serial.println("VoiceRecognition: STOP command received");
            _stateMachine->processEvent(StateMachine::EVENT_STOP_PRESSED);
            break;

        case CMD_FASTER:
            Serial.println("VoiceRecognition: FASTER command received");
            _stateMachine->processEvent(StateMachine::EVENT_SPEED_UP);
            break;

        case CMD_SLOWER:
            Serial.println("VoiceRecognition: SLOWER command received");
            _stateMachine->processEvent(StateMachine::EVENT_SPEED_DOWN);
            break;

        case CMD_DOOR:
            Serial.println("VoiceRecognition: DOOR command received");
            _stateMachine->processEvent(StateMachine::EVENT_DOOR_TOGGLE);
            break;

        default:
            Serial.println("VoiceRecognition: Unknown command");
            break;
    }
}
