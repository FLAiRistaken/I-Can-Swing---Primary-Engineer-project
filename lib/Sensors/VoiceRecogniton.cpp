// lib/Sensors/VoiceRecognition.cpp
#include "VoiceRecognition.h"
#include "Debug.h"

VoiceRecognition::VoiceRecognition(uint8_t rxPin, uint8_t txPin, StateMachine* stateMachine)
    : myVR(rxPin, txPin), _stateMachine(stateMachine) {  // ← BACK TO ORIGINAL CONSTRUCTOR

    // Initialize records array
    for (int i = 0; i < CMD_COUNT; i++) {
        records[i] = i;
    }
}

void VoiceRecognition::begin() {
    myVR.begin(9600);
    DEBUG_PRINTLN("VoiceRecognition: Initializing...");

    // Enhanced retry logic that worked before
    for (int attempt = 0; attempt < 3; attempt++) {
        if (myVR.load(records, CMD_COUNT) >= 0) {
            DEBUG_PRINTLN("VoiceRecognition: Voice commands loaded successfully");

            // Verify commands
            uint8_t checkBuf[16];
            int checkResult = myVR.checkRecognizer(checkBuf);

            if (checkResult > 0) {
                DEBUG_PRINT("VoiceRecognition: ");
                DEBUG_PRINT(checkBuf[0]);
                DEBUG_PRINTLN(" commands verified in recognizer");
                DEBUG_PRINTLN("VoiceRecognition: Ready");
                return;
            }
        } else {
            Serial.print("VoiceRecognition: Error loading commands, attempt ");
            Serial.print(attempt + 1);
            Serial.println("/3");
            delay(500);
        }
    }

    Serial.println("VoiceRecognition: FAILED to load commands after 3 attempts!");
}

void VoiceRecognition::update() {
    int ret = myVR.recognize(buf, 200);
    if (ret > 0) {
        int commandId = buf[1];
        DEBUG_PRINT("VoiceRecognition: Command recognized: ");
        DEBUG_PRINTLN(commandId);
        handleVoiceCommand(commandId);
    }
}

void VoiceRecognition::handleVoiceCommand(int command) {
    switch(command) {
        case CMD_GO:
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
        case CMD_OPEN:
            Serial.println("VoiceRecognition: DOOR OPEN command received");
            _stateMachine->processEvent(StateMachine::EVENT_DOOR_OPEN_PRESSED);
            break;
        case CMD_CLOSE:
            Serial.println("VoiceRecognition: DOOR CLOSE command received");
            _stateMachine->processEvent(StateMachine::EVENT_DOOR_CLOSE_PRESSED);
            break;
        default:
            Serial.println("VoiceRecognition: Unknown command");
            break;
    }
}
