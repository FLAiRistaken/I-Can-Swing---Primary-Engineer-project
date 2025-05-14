// test/integration_tests/test_user_presence_integration.cpp
#include <unity.h>
#include "StateMachine.h"
#include "SafetyMonitor.h"
#include "PressureSensor.h"

// Mock classes for testing
class MockPressureSensor : public PressureSensor {
public:
    MockPressureSensor() : PressureSensor(0, 0, "MockSensor") {}

    void setMockOccupied(bool occupied) { _mockOccupied = occupied; }

    bool isOccupied() override { return _mockOccupied; }

    int readRawValue() override {
        return _mockOccupied ? 800 : 200; // Above/below threshold
    }

private:
    bool _mockOccupied = false;
};

class MockUltrasonicSensor : public UltrasonicSensor {
public:
    MockUltrasonicSensor(const char* name = "Mock")
        : UltrasonicSensor(0, 0, name), _mockDistance(100.0f) {}

    float measureDistance() override { return _mockDistance; }
    void setMockDistance(float distance) { _mockDistance = distance; }

private:
    float _mockDistance;
};

// Test fixtures
StateMachine stateMachine;
MockPressureSensor pressureSensor;
MockUltrasonicSensor frontSensor("Front");
MockUltrasonicSensor rearSensor("Rear");
SafetyMonitor safetyMonitor(&stateMachine, &frontSensor, &rearSensor, &pressureSensor);

void setUp(void) {
    // Reset components before each test
    stateMachine = StateMachine();
    stateMachine.begin();

    pressureSensor = MockPressureSensor();
    frontSensor = MockUltrasonicSensor("Front");
    rearSensor = MockUltrasonicSensor("Rear");
    frontSensor.setMockDistance(100.0f);
    rearSensor.setMockDistance(100.0f);

    safetyMonitor.~SafetyMonitor();
    new (&safetyMonitor) SafetyMonitor(&stateMachine, &frontSensor, &rearSensor, &pressureSensor);
    safetyMonitor.begin();
}

void tearDown(void) {
    // Cleanup after test
}

// Test cases

void test_start_requires_user_present(void) {
    // Test that swing won't start without user pressure detected
    pressureSensor.setMockOccupied(false);

    // Process pressure sensor reading
    safetyMonitor.checkSafety();

    // Try to start swinging
    stateMachine.processEvent(StateMachine::EVENT_START_PRESSED);

    // Should remain in IDLE state
    TEST_ASSERT_EQUAL(StateMachine::STATE_IDLE, stateMachine.getCurrentState());
    TEST_ASSERT_EQUAL(StateMachine::SPEED_OFF, stateMachine.getCurrentSpeed());

    // Now simulate user sitting on swing
    pressureSensor.setMockOccupied(true);
    safetyMonitor.checkSafety();
    stateMachine.processEvent(StateMachine::EVENT_PRESSURE_ON);

    // Try to start swinging again
    stateMachine.processEvent(StateMachine::EVENT_START_PRESSED);

    // Should transition to SWINGING state
    TEST_ASSERT_EQUAL(StateMachine::STATE_SWINGING, stateMachine.getCurrentState());
    TEST_ASSERT_EQUAL(StateMachine::SPEED_LOW, stateMachine.getCurrentSpeed());
}

void test_user_leaves_during_swing(void) {
    // Setup: Start swinging with user present
    pressureSensor.setMockOccupied(true);
    safetyMonitor.checkSafety();
    stateMachine.processEvent(StateMachine::EVENT_PRESSURE_ON);
    stateMachine.processEvent(StateMachine::EVENT_START_PRESSED);

    // Verify swing started
    TEST_ASSERT_EQUAL(StateMachine::STATE_SWINGING, stateMachine.getCurrentState());

    // User leaves the swing
    pressureSensor.setMockOccupied(false);

    // Safety check should detect absence and stop swing
    SafetyMonitor::SafetyStatus status = safetyMonitor.checkSafety();

    // Verify emergency status
    TEST_ASSERT_EQUAL(SafetyMonitor::STATUS_EMERGENCY, status);

    // Verify returned to IDLE state (not EMERGENCY state, since it's user absence)
    TEST_ASSERT_EQUAL(StateMachine::STATE_IDLE, stateMachine.getCurrentState());
    TEST_ASSERT_EQUAL(StateMachine::SPEED_OFF, stateMachine.getCurrentSpeed());
}

void test_user_leaves_and_returns(void) {
    // Setup: Start swinging with user present
    pressureSensor.setMockOccupied(true);
    safetyMonitor.checkSafety();
    stateMachine.processEvent(StateMachine::EVENT_PRESSURE_ON);
    stateMachine.processEvent(StateMachine::EVENT_START_PRESSED);

    // Verify swing started
    TEST_ASSERT_EQUAL(StateMachine::STATE_SWINGING, stateMachine.getCurrentState());

    // User leaves the swing
    pressureSensor.setMockOccupied(false);
    safetyMonitor.checkSafety();

    // Swing should stop
    TEST_ASSERT_EQUAL(StateMachine::STATE_IDLE, stateMachine.getCurrentState());

    // User returns
    pressureSensor.setMockOccupied(true);
    safetyMonitor.checkSafety();
    stateMachine.processEvent(StateMachine::EVENT_PRESSURE_ON);

    // Should remain IDLE until start is pressed again
    TEST_ASSERT_EQUAL(StateMachine::STATE_IDLE, stateMachine.getCurrentState());

    // Press start again
    stateMachine.processEvent(StateMachine::EVENT_START_PRESSED);

    // Should start swinging again
    TEST_ASSERT_EQUAL(StateMachine::STATE_SWINGING, stateMachine.getCurrentState());
}

void test_user_presence_during_door_operation(void) {
    // User presence shouldn't affect door operations
    pressureSensor.setMockOccupied(false); // User not present

    // Try door operation
    stateMachine.processEvent(StateMachine::EVENT_DOOR_TOGGLE);

    // Should transition to DOOR_OPENING regardless
    TEST_ASSERT_EQUAL(StateMachine::STATE_DOOR_OPENING, stateMachine.getCurrentState());
}

void test_continuous_pressure_monitoring(void) {
    // Setup: Start swinging with user present
    pressureSensor.setMockOccupied(true);
    safetyMonitor.checkSafety();
    stateMachine.processEvent(StateMachine::EVENT_PRESSURE_ON);
    stateMachine.processEvent(StateMachine::EVENT_START_PRESSED);

    // Run multiple safety checks with user present
    for (int i = 0; i < 5; i++) {
        pressureSensor.setMockOccupied(true);
        safetyMonitor.checkSafety();
        TEST_ASSERT_EQUAL(StateMachine::STATE_SWINGING, stateMachine.getCurrentState());
    }

    // Simulate brief pressure fluctuations (should be tolerated)
    pressureSensor.setMockOccupied(false);
    safetyMonitor.checkSafety();

    // Should stop swinging immediately
    TEST_ASSERT_EQUAL(StateMachine::STATE_IDLE, stateMachine.getCurrentState());
}

void run_user_presence_integration_tests(void) {
    UNITY_BEGIN();
    RUN_TEST(test_start_requires_user_present);
    RUN_TEST(test_user_leaves_during_swing);
    RUN_TEST(test_user_leaves_and_returns);
    RUN_TEST(test_user_presence_during_door_operation);
    RUN_TEST(test_continuous_pressure_monitoring);
    UNITY_END();
}

void setup() {
    delay(2000);
    run_user_presence_integration_tests();
}

void loop() {
    // Nothing to do here
}
