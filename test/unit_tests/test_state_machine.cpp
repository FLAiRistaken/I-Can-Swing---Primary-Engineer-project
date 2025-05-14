// test/unit_tests/test_state_machine.cpp
#include <unity.h>
#include "StateMachine.h"
#include "ActuatorDriver.h"
#include "BuzzerDriver.h"

// Mock classes for dependencies
class MockBuzzer : public BuzzerDriver {
public:
    MockBuzzer() : BuzzerDriver(0) {}
    void beep(uint16_t freq = 1000, uint32_t duration = 100) override {
        beepCalled = true;
        lastFreq = freq;
        lastDuration = duration;
    }
    bool beepCalled = false;
    uint16_t lastFreq = 0;
    uint32_t lastDuration = 0;
};

class MockActuator : public ActuatorDriver {
public:
    MockActuator() : ActuatorDriver(0, 0, nullptr) {}
    void startExtend(unsigned long timeMs = DOOR_OPEN_TIME_MS) override {
        extendCalled = true;
    }
    void startRetract(unsigned long timeMs = DOOR_OPEN_TIME_MS) override {
        retractCalled = true;
    }
    void stop() override {
        stopCalled = true;
    }
    bool extendCalled = false;
    bool retractCalled = false;
    bool stopCalled = false;
};

// Test fixtures
StateMachine stateMachine;
MockBuzzer mockBuzzer;
MockActuator mockActuator;

void setUp(void) {
    // Reset state machine and mocks before each test
    mockBuzzer.beepCalled = false;
    mockBuzzer.lastFreq = 0;
    mockBuzzer.lastDuration = 0;
    mockActuator.extendCalled = false;
    mockActuator.retractCalled = false;
    mockActuator.stopCalled = false;

    stateMachine = StateMachine();
    stateMachine.setBuzzer(&mockBuzzer);
    stateMachine.setDoorActuator(&mockActuator);
    stateMachine.begin();
}

void tearDown(void) {
    // Cleanup after each test
}

// Test cases
void test_initial_state(void) {
    TEST_ASSERT_EQUAL(StateMachine::STATE_IDLE, stateMachine.getCurrentState());
    TEST_ASSERT_EQUAL(StateMachine::SPEED_OFF, stateMachine.getCurrentSpeed());
}

void test_start_transition_requires_user(void) {
    // Without user present, should not transition
    stateMachine.processEvent(StateMachine::EVENT_START_PRESSED);
    TEST_ASSERT_EQUAL(StateMachine::STATE_IDLE, stateMachine.getCurrentState());

    // With user present, should transition to SWINGING
    stateMachine.processEvent(StateMachine::EVENT_PRESSURE_ON);
    stateMachine.processEvent(StateMachine::EVENT_START_PRESSED);
    TEST_ASSERT_EQUAL(StateMachine::STATE_SWINGING, stateMachine.getCurrentState());
    TEST_ASSERT_EQUAL(StateMachine::SPEED_LOW, stateMachine.getCurrentSpeed());
}

void test_stop_from_swinging(void) {
    // Setup swinging state
    stateMachine.processEvent(StateMachine::EVENT_PRESSURE_ON);
    stateMachine.processEvent(StateMachine::EVENT_START_PRESSED);

    // Stop should return to IDLE
    stateMachine.processEvent(StateMachine::EVENT_STOP_PRESSED);
    TEST_ASSERT_EQUAL(StateMachine::STATE_IDLE, stateMachine.getCurrentState());
    TEST_ASSERT_EQUAL(StateMachine::SPEED_OFF, stateMachine.getCurrentSpeed());
}

void test_user_leaves_during_swing(void) {
    // Setup swinging state
    stateMachine.processEvent(StateMachine::EVENT_PRESSURE_ON);
    stateMachine.processEvent(StateMachine::EVENT_START_PRESSED);

    // User leaving should stop swing
    stateMachine.processEvent(StateMachine::EVENT_PRESSURE_OFF);
    TEST_ASSERT_EQUAL(StateMachine::STATE_IDLE, stateMachine.getCurrentState());
    TEST_ASSERT_EQUAL(StateMachine::SPEED_OFF, stateMachine.getCurrentSpeed());
}

void test_speed_changes(void) {
    // Setup swinging state at LOW speed
    stateMachine.processEvent(StateMachine::EVENT_PRESSURE_ON);
    stateMachine.processEvent(StateMachine::EVENT_START_PRESSED);
    TEST_ASSERT_EQUAL(StateMachine::SPEED_LOW, stateMachine.getCurrentSpeed());

    // Increase to MEDIUM
    stateMachine.processEvent(StateMachine::EVENT_SPEED_UP);
    TEST_ASSERT_EQUAL(StateMachine::SPEED_MEDIUM, stateMachine.getCurrentSpeed());

    // Increase to HIGH
    stateMachine.processEvent(StateMachine::EVENT_SPEED_UP);
    TEST_ASSERT_EQUAL(StateMachine::SPEED_HIGH, stateMachine.getCurrentSpeed());

    // Try to increase beyond MAX (should remain HIGH)
    stateMachine.processEvent(StateMachine::EVENT_SPEED_UP);
    TEST_ASSERT_EQUAL(StateMachine::SPEED_HIGH, stateMachine.getCurrentSpeed());

    // Decrease to MEDIUM
    stateMachine.processEvent(StateMachine::EVENT_SPEED_DOWN);
    TEST_ASSERT_EQUAL(StateMachine::SPEED_MEDIUM, stateMachine.getCurrentSpeed());

    // Decrease to LOW
    stateMachine.processEvent(StateMachine::EVENT_SPEED_DOWN);
    TEST_ASSERT_EQUAL(StateMachine::SPEED_LOW, stateMachine.getCurrentSpeed());

    // Try to decrease below MIN (should remain LOW)
    stateMachine.processEvent(StateMachine::EVENT_SPEED_DOWN);
    TEST_ASSERT_EQUAL(StateMachine::SPEED_LOW, stateMachine.getCurrentSpeed());
}

void test_emergency_handling(void) {
    // From IDLE state
    stateMachine.processEvent(StateMachine::EVENT_EMERGENCY);
    TEST_ASSERT_EQUAL(StateMachine::STATE_EMERGENCY, stateMachine.getCurrentState());
    TEST_ASSERT_TRUE(mockBuzzer.beepCalled);

    // Only emergency reset should work
    stateMachine.processEvent(StateMachine::EVENT_START_PRESSED);
    TEST_ASSERT_EQUAL(StateMachine::STATE_EMERGENCY, stateMachine.getCurrentState());

    stateMachine.processEvent(StateMachine::EVENT_EMERGENCY_RESET);
    TEST_ASSERT_EQUAL(StateMachine::STATE_IDLE, stateMachine.getCurrentState());
}

void test_emergency_from_swinging(void) {
    // Setup swinging state
    stateMachine.processEvent(StateMachine::EVENT_PRESSURE_ON);
    stateMachine.processEvent(StateMachine::EVENT_START_PRESSED);

    // Emergency should override everything
    stateMachine.processEvent(StateMachine::EVENT_EMERGENCY);
    TEST_ASSERT_EQUAL(StateMachine::STATE_EMERGENCY, stateMachine.getCurrentState());
    TEST_ASSERT_EQUAL(StateMachine::SPEED_OFF, stateMachine.getCurrentSpeed());
}

void test_obstacle_detection_during_swing(void) {
    // Setup swinging state
    stateMachine.processEvent(StateMachine::EVENT_PRESSURE_ON);
    stateMachine.processEvent(StateMachine::EVENT_START_PRESSED);

    // Obstacle should trigger ERROR state
    stateMachine.processEvent(StateMachine::EVENT_OBSTACLE_DETECTED);
    TEST_ASSERT_EQUAL(StateMachine::STATE_ERROR, stateMachine.getCurrentState());

    // Clear error should return to IDLE
    stateMachine.processEvent(StateMachine::EVENT_ERROR_CLEARED);
    TEST_ASSERT_EQUAL(StateMachine::STATE_IDLE, stateMachine.getCurrentState());
}

void test_door_operations(void) {
    // Test door opening
    stateMachine.processEvent(StateMachine::EVENT_DOOR_TOGGLE);
    TEST_ASSERT_EQUAL(StateMachine::STATE_DOOR_OPENING, stateMachine.getCurrentState());
    TEST_ASSERT_TRUE(mockActuator.extendCalled);

    // Door fully open
    stateMachine.processEvent(StateMachine::EVENT_DOOR_OPENED);
    TEST_ASSERT_EQUAL(StateMachine::STATE_IDLE, stateMachine.getCurrentState());

    // Test door closing
    stateMachine.processEvent(StateMachine::EVENT_DOOR_TOGGLE);
    stateMachine.processEvent(StateMachine::EVENT_DOOR_TOGGLE); // Toggle again to close
    TEST_ASSERT_EQUAL(StateMachine::STATE_DOOR_CLOSING, stateMachine.getCurrentState());
    TEST_ASSERT_TRUE(mockActuator.retractCalled);

    // Door fully closed
    stateMachine.processEvent(StateMachine::EVENT_DOOR_CLOSED);
    TEST_ASSERT_EQUAL(StateMachine::STATE_IDLE, stateMachine.getCurrentState());
}

void test_door_obstacle_detection(void) {
    // Open door
    stateMachine.processEvent(StateMachine::EVENT_DOOR_TOGGLE);
    TEST_ASSERT_EQUAL(StateMachine::STATE_DOOR_OPENING, stateMachine.getCurrentState());

    // Detect obstacle
    stateMachine.processEvent(StateMachine::EVENT_OBSTACLE_DETECTED);
    TEST_ASSERT_EQUAL(StateMachine::STATE_ERROR, stateMachine.getCurrentState());
    TEST_ASSERT_TRUE(mockActuator.stopCalled);
}

void test_door_timeout(void) {
    // Set very short timeout for testing
    stateMachine.setDoorTimeout(10);

    // Start door opening
    stateMachine.processEvent(StateMachine::EVENT_DOOR_TOGGLE);
    TEST_ASSERT_EQUAL(StateMachine::STATE_DOOR_OPENING, stateMachine.getCurrentState());

    // Wait for timeout
    delay(15);
    stateMachine.update(); // This should trigger timeout

    TEST_ASSERT_EQUAL(StateMachine::STATE_ERROR, stateMachine.getCurrentState());
    TEST_ASSERT_TRUE(mockActuator.stopCalled);
}

void run_state_machine_tests() {
    UNITY_BEGIN();
    RUN_TEST(test_initial_state);
    RUN_TEST(test_start_transition_requires_user);
    RUN_TEST(test_stop_from_swinging);
    RUN_TEST(test_user_leaves_during_swing);
    RUN_TEST(test_speed_changes);
    RUN_TEST(test_emergency_handling);
    RUN_TEST(test_emergency_from_swinging);
    RUN_TEST(test_obstacle_detection_during_swing);
    RUN_TEST(test_door_operations);
    RUN_TEST(test_door_obstacle_detection);
    RUN_TEST(test_door_timeout);
    UNITY_END();
}

void setup() {
    delay(2000);
    run_state_machine_tests();
}

void loop() {
    // Nothing to do here
}
