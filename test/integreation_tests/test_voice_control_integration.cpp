// test/integration_tests/test_voice_control_integration.cpp
#include <unity.h>
#include "VoiceRecognition.h"
#include "StateMachine.h"
#include "StepperDriver.h"
#include "PressureSensor.h"

// Mock classes for testing
class MockVR {
public:
    MockVR(uint8_t rxPin, uint8_t txPin) {}

    bool begin(long baudrate) { return true; }
    bool load(uint8_t* records, uint8_t count) { return true; }

    int recognize(uint8_t* buf, int timeout) {
        if (_simulateCommand) {
            buf[0] = 0;
            buf[1] = _commandToSimulate;
            _simulateCommand = false;
            return 2;
        }
        return 0;
    }

    void simulateCommand(int command) {
        _simulateCommand = true;
        _commandToSimulate = command;
    }

private:
    bool _simulateCommand = false;
    int _commandToSimulate = 0;
};

class MockPressureSensor : public PressureSensor {
public:
    MockPressureSensor() : PressureSensor(0, 0, "Mock") {}
    bool isOccupied() override { return _mockOccupied; }
    void setMockOccupied(bool occupied) { _mockOccupied = occupied; }
private:
    bool _mockOccupied = false;
};

// Override VR with our mock
#define VR MockVR

// Test fixtures
StateMachine stateMachine;
StepperDriver leftMotor(3, 4, 5, 6);
StepperDriver rightMotor(6, 5, 4, 3); // Reversed pins for opposite rotation
VoiceRecognition* voiceRecognition;
MockPressureSensor mockPressure;

// Helper function to update motors based on state machine state
void updateMotors() {
    if (stateMachine.getCurrentState() == StateMachine::STATE_SWINGING) {
        uint16_t speed = 0;
        switch (stateMachine.getCurrentSpeed()) {
            case StateMachine::SPEED_LOW: speed = 30; break;
            case StateMachine::SPEED_MEDIUM: speed = 60; break;
            case StateMachine::SPEED_HIGH: speed = 90; break;
            default: speed = 0;
        }

        leftMotor.setSpeed(speed);
        rightMotor.setSpeed(speed);

        if (speed > 0) {
            if (!leftMotor.isRunning()) {
                leftMotor.enable();
                rightMotor.enable();
                leftMotor.startContinuous();
                rightMotor.startContinuous();
            }
        } else {
            leftMotor.stop();
            rightMotor.stop();
        }
    } else {
        leftMotor.stop();
        rightMotor.stop();
    }
}

void setUp(void) {
    stateMachine = StateMachine();
    stateMachine.begin();

    leftMotor = StepperDriver(3, 4, 5, 6);
    leftMotor.begin();

    rightMotor = StepperDriver(6, 5, 4, 3);
    rightMotor.begin();

    mockPressure.setMockOccupied(false);

    voiceRecognition = new VoiceRecognition(0, 1, &stateMachine);
    voiceRecognition->begin();
}

void tearDown(void) {
    delete voiceRecognition;
}

// Test cases
void test_go_command_starts_motors() {
    // Setup: User present in swing
    mockPressure.setMockOccupied(true);
    stateMachine.processEvent(StateMachine::EVENT_PRESSURE_ON);

    // Verify initial state
    TEST_ASSERT_EQUAL(StateMachine::STATE_IDLE, stateMachine.getCurrentState());
    TEST_ASSERT_FALSE(leftMotor.isRunning());
    TEST_ASSERT_FALSE(rightMotor.isRunning());

    // Simulate GO voice command
    MockVR& vr = (MockVR&)voiceRecognition->myVR;
    vr.simulateCommand(VoiceRecognition::CMD_GO);
    voiceRecognition->update();

    // Update motors based on state
    updateMotors();

    // Verify state transition and motors running
    TEST_ASSERT_EQUAL(StateMachine::STATE_SWINGING, stateMachine.getCurrentState());
    TEST_ASSERT_EQUAL(StateMachine::SPEED_LOW, stateMachine.getCurrentSpeed());
    TEST_ASSERT_TRUE(leftMotor.isRunning());
    TEST_ASSERT_TRUE(rightMotor.isRunning());
}

void test_stop_command_stops_motors() {
    // Setup: Start swinging
    mockPressure.setMockOccupied(true);
    stateMachine.processEvent(StateMachine::EVENT_PRESSURE_ON);
    stateMachine.processEvent(StateMachine::EVENT_START_PRESSED);
    updateMotors();

    // Verify motors running
    TEST_ASSERT_TRUE(leftMotor.isRunning());
    TEST_ASSERT_TRUE(rightMotor.isRunning());

    // Simulate STOP voice command
    MockVR& vr = (MockVR&)voiceRecognition->myVR;
    vr.simulateCommand(VoiceRecognition::CMD_STOP);
    voiceRecognition->update();
    updateMotors();

    // Verify motors stopped
    TEST_ASSERT_EQUAL(StateMachine::STATE_IDLE, stateMachine.getCurrentState());
    TEST_ASSERT_FALSE(leftMotor.isRunning());
    TEST_ASSERT_FALSE(rightMotor.isRunning());
}

void test_faster_command_increases_speed() {
    // Setup: Start swinging at low speed
    mockPressure.setMockOccupied(true);
    stateMachine.processEvent(StateMachine::EVENT_PRESSURE_ON);
    stateMachine.processEvent(StateMachine::EVENT_START_PRESSED);
    updateMotors();

    uint16_t initialSpeed = leftMotor.getSpeed(); // Access for testing

    // Simulate FASTER voice command
    MockVR& vr = (MockVR&)voiceRecognition->myVR;
    vr.simulateCommand(VoiceRecognition::CMD_FASTER);
    voiceRecognition->update();
    updateMotors();

    // Verify speed increased
    TEST_ASSERT_EQUAL(StateMachine::SPEED_MEDIUM, stateMachine.getCurrentSpeed());
    TEST_ASSERT_GREATER_THAN(initialSpeed, leftMotor.getSpeed());
}

void test_slower_command_decreases_speed() {
    // Setup: Start swinging and increase to medium speed
    mockPressure.setMockOccupied(true);
    stateMachine.processEvent(StateMachine::EVENT_PRESSURE_ON);
    stateMachine.processEvent(StateMachine::EVENT_START_PRESSED);
    stateMachine.processEvent(StateMachine::EVENT_SPEED_UP);
    updateMotors();

    uint16_t mediumSpeed = leftMotor.getSpeed();

    // Simulate SLOWER voice command
    MockVR& vr = (MockVR&)voiceRecognition->myVR;
    vr.simulateCommand(VoiceRecognition::CMD_SLOWER);
    voiceRecognition->update();
    updateMotors();

    // Verify speed decreased
    TEST_ASSERT_EQUAL(StateMachine::SPEED_LOW, stateMachine.getCurrentSpeed());
    TEST_ASSERT_LESS_THAN(mediumSpeed, leftMotor.getSpeed());
}

void test_command_sequence() {
    // Setup
    mockPressure.setMockOccupied(true);
    stateMachine.processEvent(StateMachine::EVENT_PRESSURE_ON);
    MockVR& vr = (MockVR&)voiceRecognition->myVR;

    // Sequence: GO → FASTER → FASTER → SLOWER → STOP
    // 1. GO command
    vr.simulateCommand(VoiceRecognition::CMD_GO);
    voiceRecognition->update();
    updateMotors();
    TEST_ASSERT_TRUE(leftMotor.isRunning());
    TEST_ASSERT_EQUAL(StateMachine::SPEED_LOW, stateMachine.getCurrentSpeed());

    // 2. FASTER command (to MEDIUM)
    vr.simulateCommand(VoiceRecognition::CMD_FASTER);
    voiceRecognition->update();
    updateMotors();
    TEST_ASSERT_EQUAL(StateMachine::SPEED_MEDIUM, stateMachine.getCurrentSpeed());

    // 3. FASTER command (to HIGH)
    vr.simulateCommand(VoiceRecognition::CMD_FASTER);
    voiceRecognition->update();
    updateMotors();
    TEST_ASSERT_EQUAL(StateMachine::SPEED_HIGH, stateMachine.getCurrentSpeed());

    // 4. SLOWER command (to MEDIUM)
    vr.simulateCommand(VoiceRecognition::CMD_SLOWER);
    voiceRecognition->update();
    updateMotors();
    TEST_ASSERT_EQUAL(StateMachine::SPEED_MEDIUM, stateMachine.getCurrentSpeed());

    // 5. STOP command
    vr.simulateCommand(VoiceRecognition::CMD_STOP);
    voiceRecognition->update();
    updateMotors();
    TEST_ASSERT_FALSE(leftMotor.isRunning());
    TEST_ASSERT_EQUAL(StateMachine::STATE_IDLE, stateMachine.getCurrentState());
}

void run_voice_motor_integration_tests() {
    UNITY_BEGIN();
    RUN_TEST(test_go_command_starts_motors);
    RUN_TEST(test_stop_command_stops_motors);
    RUN_TEST(test_faster_command_increases_speed);
    RUN_TEST(test_slower_command_decreases_speed);
    RUN_TEST(test_command_sequence);
    UNITY_END();
}

void setup() {
    delay(2000);
    run_voice_motor_integration_tests();
}

void loop() {
    // Nothing to do here
}
