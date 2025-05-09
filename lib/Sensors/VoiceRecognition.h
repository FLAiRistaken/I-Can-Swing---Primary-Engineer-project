// lib/Sensors/VoiceRecognition.h
#pragma once

#include <Arduino.h>
#include <SoftwareSerial.h>
#include "VoiceRecognitionV3.h"
#include "StateMachine.h"

class VoiceRecognition {
public:
    // Voice commands (must match indices used during training)
    enum Command {
        CMD_START = 0,
        CMD_STOP = 1,
        CMD_FASTER = 2,
        CMD_SLOWER = 3,
        CMD_DOOR = 4,
        CMD_COUNT = 5  // Total number of commands
    };

    VoiceRecognition(uint8_t rxPin, uint8_t txPin, StateMachine* stateMachine);
    void begin();
    void update();  // Call this in loop() to process voice commands

private:
    VR myVR;  // Voice recognition object
    StateMachine* _stateMachine;
    uint8_t buf[64];  // Buffer for results

    // Records for commands (must be trained)
    uint8_t records[CMD_COUNT];

    void handleVoiceCommand(int command);
};
