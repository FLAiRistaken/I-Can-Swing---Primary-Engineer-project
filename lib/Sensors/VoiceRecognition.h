// lib/Sensors/VoiceRecognition.h
#pragma once

#include <Arduino.h>
#include <SoftwareSerial.h>
#include "VoiceRecognitionV3.h"
#include "StateMachine.h"

class MockVR;

class VoiceRecognition {
    friend class MockVR;
    friend void test_initialization();
    friend void test_failed_initialization();
    friend void test_update_no_command();
    friend void test_go_command();
    friend void test_stop_command();
    friend void test_faster_command();
    friend void test_slower_command();
    friend void test_open_command();
    friend void test_close_command();
    friend void test_unknown_command();
    friend void test_multiple_commands();
    friend void test_go_command_starts_motors();
    friend void test_stop_command_stops_motors();
    friend void test_faster_command_increases_speed();
    friend void test_slower_command_decreases_speed();
    friend void test_command_sequence();
public:
    // Voice commands (must match indices used during training)
    enum Command {
        CMD_GO = 0,
        CMD_STOP = 1,
        CMD_FASTER = 2,
        CMD_SLOWER = 3,
        CMD_OPEN = 4,
        CMD_CLOSE = 5,
        CMD_COUNT = 6  // Total number of commands
    };

    VoiceRecognition(uint8_t rxPin, uint8_t txPin, StateMachine* stateMachine);
    void begin();
    virtual void update();  // Call this in loop() to process voice commands

private:
    VR myVR;  // Voice recognition object
    StateMachine* _stateMachine;
    uint8_t buf[64];  // Buffer for results

    // Records for commands (must be trained)
    uint8_t records[CMD_COUNT];

protected:
    virtual void handleVoiceCommand(int command);
};

