// test/unit_tests/test_voice_recognition.cpp
#include <unity.h>
#include "VoiceRecognition.h"
#include "StateMachine.h"
#include "Configuration.h"

// Mock VR library to simulate voice recognition
class MockVR {
public:
    MockVR(uint8_t rxPin, uint8_t txPin) : _rxPin(rxPin), _txPin(txPin), _beginCalled(false),
                                          _loadCalled(false), _recognizeCalled(false) {
        // Simulate successful initialization
    }

    bool begin(long baudrate) {
        _beginCalled = true;
        _baudrate = baudrate;
        return true;
    }

    bool load(uint8_t* records, uint8_t count) {
        _loadCalled = true;
        _loadCount = count;
        // Copy record IDs
        for (int i = 0; i < count && i < 10; i++) {
            _loadedRecords[i] = records[i];
        }
        return _loadShouldSucceed;
    }

    int recognize(uint8_t* buf, int timeout) {
        _recognizeCalled = true;
        _recognizeTimeout = timeout;

        if (!_simulateRecognition) return 0;

        // Simulate a recognition result
        buf[0] = 0; // Header
        buf[1] = _simulatedCommand; // Command ID
        return 2; // Number of bytes read
    }

    // Test control methods
    void setLoadSuccess(bool success) { _loadShouldSucceed = success; }
    void simulateCommand(int command) {
        _simulateRecognition = true;
        _simulatedCommand = command;
    }
    void clearSimulation() { _simulateRecognition = false; }

    // Inspection methods
    bool wasBeginCalled() const { return _beginCalled; }
    bool wasLoadCalled() const { return _loadCalled; }
    bool wasRecognizeCalled() const { return _recognizeCalled; }
    int getLoadCount() const { return _loadCount; }

private:
    uint8_t _rxPin;
    uint8_t _txPin;
    long _baudrate;
    bool _beginCalled;
    bool _loadCalled;
    bool _recognizeCalled;
    int _recognizeTimeout;
    int _loadCount;
    uint8_t _loadedRecords[10];
    bool _loadShouldSucceed = true;
    bool _simulateRecognition = false;
    uint8_t _simulatedCommand = 0;
};

// Mock StateMachine to verify events
class MockStateMachine : public StateMachine {
public:
    MockStateMachine() : _lastEvent(EVENT_NONE), _eventCount(0) {}

    void processEvent(Event event) override {
        _lastEvent = event;
        _eventHistory[_eventCount % 10] = event;
        _eventCount++;
    }

    // Test inspection methods
    Event getLastEvent() const { return _lastEvent; }
    int getEventCount() const { return _eventCount; }

    void reset() {
        _lastEvent = EVENT_NONE;
        _eventCount = 0;
    }

private:
    Event _lastEvent;
    Event _eventHistory[10];
    int _eventCount;
};

// Replace the standard VR class with our mock for testing
#define VR MockVR

// Test fixtures
MockStateMachine mockStateMachine;
VoiceRecognition* voiceRecognition;

void setUp(void) {
    // Create fresh instance for each test
    mockStateMachine.reset();
    voiceRecognition = new VoiceRecognition(PIN_VOICE_RX, PIN_VOICE_TX, &mockStateMachine);
}

void tearDown(void) {
    delete voiceRecognition;
}

// Test cases
void test_initialization() {
    voiceRecognition->begin();

    auto& vr = reinterpret_cast<MockVR&>(voiceRecognition->myVR);
    TEST_ASSERT_TRUE(vr.wasBeginCalled());
    TEST_ASSERT_TRUE(vr.wasLoadCalled());
    TEST_ASSERT_EQUAL(VoiceRecognition::CMD_COUNT, vr.getLoadCount());
}

void test_failed_initialization() {
    auto& vr = reinterpret_cast<MockVR&>(voiceRecognition->myVR);
    vr.setLoadSuccess(false);

    voiceRecognition->begin();

    TEST_ASSERT_TRUE(vr.wasBeginCalled());
    TEST_ASSERT_TRUE(vr.wasLoadCalled());
    // Even with failed load, it should continue
}

void test_update_no_command() {
    voiceRecognition->begin();

    // No command simulated
    voiceRecognition->update();

    // Should check for commands
    auto& vr = reinterpret_cast<MockVR&>(voiceRecognition->myVR);
    TEST_ASSERT_TRUE(vr.wasRecognizeCalled());

    // But no event should be sent to state machine
    TEST_ASSERT_EQUAL(0, mockStateMachine.getEventCount());
}

void test_go_command() {
    voiceRecognition->begin();

    // Simulate GO command
    auto& vr = reinterpret_cast<MockVR&>(voiceRecognition->myVR);
    vr.simulateCommand(VoiceRecognition::CMD_GO);

    voiceRecognition->update();

    TEST_ASSERT_EQUAL(StateMachine::EVENT_START_PRESSED, mockStateMachine.getLastEvent());
}

void test_stop_command() {
    voiceRecognition->begin();

    // Simulate STOP command
    auto& vr = reinterpret_cast<MockVR&>(voiceRecognition->myVR);
    vr.simulateCommand(VoiceRecognition::CMD_STOP);

    voiceRecognition->update();

    TEST_ASSERT_EQUAL(StateMachine::EVENT_STOP_PRESSED, mockStateMachine.getLastEvent());
}

void test_faster_command() {
    voiceRecognition->begin();

    // Simulate FASTER command
    auto& vr = reinterpret_cast<MockVR&>(voiceRecognition->myVR);
    vr.simulateCommand(VoiceRecognition::CMD_FASTER);

    voiceRecognition->update();

    TEST_ASSERT_EQUAL(StateMachine::EVENT_SPEED_UP, mockStateMachine.getLastEvent());
}

void test_slower_command() {
    voiceRecognition->begin();

    // Simulate SLOWER command
    auto& vr = reinterpret_cast<MockVR&>(voiceRecognition->myVR);
    vr.simulateCommand(VoiceRecognition::CMD_SLOWER);

    voiceRecognition->update();

    TEST_ASSERT_EQUAL(StateMachine::EVENT_SPEED_DOWN, mockStateMachine.getLastEvent());
}

void test_open_command() {
    voiceRecognition->begin();

    // Simulate OPEN command
    auto& vr = reinterpret_cast<MockVR&>(voiceRecognition->myVR);
    vr.simulateCommand(VoiceRecognition::CMD_OPEN);

    voiceRecognition->update();

    TEST_ASSERT_EQUAL(StateMachine::EVENT_DOOR_TOGGLE, mockStateMachine.getLastEvent());
}

void test_close_command() {
    voiceRecognition->begin();

    // Simulate CLOSE command
    auto& vr = reinterpret_cast<MockVR&>(voiceRecognition->myVR);
    vr.simulateCommand(VoiceRecognition::CMD_CLOSE);

    voiceRecognition->update();

    TEST_ASSERT_EQUAL(StateMachine::EVENT_DOOR_TOGGLE, mockStateMachine.getLastEvent());
}

void test_unknown_command() {
    voiceRecognition->begin();

    // Simulate unknown command ID
    auto& vr = reinterpret_cast<MockVR&>(voiceRecognition->myVR);
    vr.simulateCommand(99); // Not defined in Command enum

    voiceRecognition->update();

    // Should not generate any event
    TEST_ASSERT_EQUAL(0, mockStateMachine.getEventCount());
}

void test_multiple_commands() {
    voiceRecognition->begin();
    auto& vr = reinterpret_cast<MockVR&>(voiceRecognition->myVR);

    // Simulate sequence: GO, FASTER, FASTER, STOP
    vr.simulateCommand(VoiceRecognition::CMD_GO);
    voiceRecognition->update();
    TEST_ASSERT_EQUAL(StateMachine::EVENT_START_PRESSED, mockStateMachine.getLastEvent());

    vr.simulateCommand(VoiceRecognition::CMD_FASTER);
    voiceRecognition->update();
    TEST_ASSERT_EQUAL(StateMachine::EVENT_SPEED_UP, mockStateMachine.getLastEvent());

    vr.simulateCommand(VoiceRecognition::CMD_FASTER);
    voiceRecognition->update();
    TEST_ASSERT_EQUAL(StateMachine::EVENT_SPEED_UP, mockStateMachine.getLastEvent());

    vr.simulateCommand(VoiceRecognition::CMD_STOP);
    voiceRecognition->update();
    TEST_ASSERT_EQUAL(StateMachine::EVENT_STOP_PRESSED, mockStateMachine.getLastEvent());

    TEST_ASSERT_EQUAL(4, mockStateMachine.getEventCount());
}

void run_voice_recognition_tests() {
    UNITY_BEGIN();
    RUN_TEST(test_initialization);
    RUN_TEST(test_failed_initialization);
    RUN_TEST(test_update_no_command);
    RUN_TEST(test_go_command);
    RUN_TEST(test_stop_command);
    RUN_TEST(test_faster_command);
    RUN_TEST(test_slower_command);
    RUN_TEST(test_open_command);
    RUN_TEST(test_close_command);
    RUN_TEST(test_unknown_command);
    RUN_TEST(test_multiple_commands);
    UNITY_END();
}

void setup() {
    delay(2000);
    run_voice_recognition_tests();
}

void loop() {
    // Nothing to do here
}
