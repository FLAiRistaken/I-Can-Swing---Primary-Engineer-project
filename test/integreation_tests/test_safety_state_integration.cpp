// test/integration_tests/test_safety_state_integration.cpp
#include <unity.h>
#include "SafetyMonitor.h"
#include "StateMachine.h"
#include "UltrasonicSensor.h"
#include "PressureSensor.h"

// Mock sensor classes to simulate safety conditions
class MockUltrasonicSensor : public UltrasonicSensor {
public:
    MockUltrasonicSensor(const char* name = "Mock")
        : UltrasonicSensor(0, 0, name), _mockDistance(100.0f) {}

    float measureDistance() override {
        return _mockDistance;
    }

    void setMockDistance(float distance) {
        _mockDistance = distance;
    }

private:
    float _mockDistance;
};

class MockPressureSensor : public PressureSensor {
public:
    MockPressureSensor(const char* name = "Mock")
        : PressureSensor(0, 0, name), _mockOccupied(false) {}

    bool isOccupied() override {
        return _mockOccupied;
    }

    void setMockOccupied(bool occupied) {
        _mockOccupied = occupied;
    }

private:
    bool _mockOccupied;
};

// Test fixtures
StateMachine stateMachine;
MockUltrasonicSensor frontSensor("FrontMock");
MockUltrasonicSensor rearSensor("RearMock");
MockPressureSensor pressureSensor("PressureMock");
SafetyMonitor safetyMonitor(&stateMachine, &frontSensor, &rearSensor, &pressureSensor);

void setUp(void) {
    // Reset components before each test
    stateMachine = StateMachine();
    stateMachine.begin();

    frontSensor.setMockDistance(100.0f);
    rearSensor.setMockDistance(100.0f);
    pressureSensor.setMockOccupied(false);

    safetyMonitor.begin();
}

void tearDown(void) {
    // Cleanup after test
}

// Test Cases

void test_obstacle_detection_to_emergency_state(void) {
    // Setup: Start swinging with user present
    pressureSensor.setMockOccupied(true);
    stateMachine.processEvent(StateMachine::EVENT_PRESSURE_ON);
    stateMachine.processEvent(StateMachine::EVENT_START_PRESSED);

    // Verify swing started
    TEST_ASSERT_EQUAL(StateMachine::STATE_SWINGING, stateMachine.getCurrentState());

    // Simulate critical obstacle detection (distance < 10cm)
    frontSensor.setMockDistance(5.0f);

    // Run safety check - should trigger emergency
    SafetyMonitor::SafetyStatus status = safetyMonitor.checkSafety();

    // Verify emergency status and state machine transition
    TEST_ASSERT_EQUAL(SafetyMonitor::STATUS_EMERGENCY, status);
    TEST_ASSERT_EQUAL(StateMachine::STATE_EMERGENCY, stateMachine.getCurrentState());
}

void test_user_absence_during_swing(void) {
    // Setup: Start swinging with user present
    pressureSensor.setMockOccupied(true);
    stateMachine.processEvent(StateMachine::EVENT_PRESSURE_ON);
    stateMachine.processEvent(StateMachine::EVENT_START_PRESSED);

    // Verify swing started
    TEST_ASSERT_EQUAL(StateMachine::STATE_SWINGING, stateMachine.getCurrentState());

    // User leaves the swing
    pressureSensor.setMockOccupied(false);

    // Run safety check
    SafetyMonitor::SafetyStatus status = safetyMonitor.checkSafety();

    // Verify emergency status and state transition to IDLE
    TEST_ASSERT_EQUAL(SafetyMonitor::STATUS_EMERGENCY, status);
    TEST_ASSERT_EQUAL(StateMachine::STATE_IDLE, stateMachine.getCurrentState());
}

void test_motor_stall_detection(void) {
    // Setup: Start swinging with user present
    pressureSensor.setMockOccupied(true);
    stateMachine.processEvent(StateMachine::EVENT_PRESSURE_ON);
    stateMachine.processEvent(StateMachine::EVENT_START_PRESSED);

    // Simulate motor stall (active but no speed)
    safetyMonitor.updateMotorStatus(true, 500); // Initial speed
    safetyMonitor.updateMotorStatus(true, 0);   // First stall
    safetyMonitor.updateMotorStatus(true, 0);   // Second stall
    safetyMonitor.updateMotorStatus(true, 0);   // Third stall

    // Run motor operation check
    SafetyMonitor::SafetyStatus status = safetyMonitor.checkMotorOperation();
    TEST_ASSERT_EQUAL(SafetyMonitor::STATUS_EMERGENCY, status);

    // Run full safety check to trigger state machine
    safetyMonitor.checkSafety();

    // Verify state transition
    TEST_ASSERT_EQUAL(StateMachine::STATE_EMERGENCY, stateMachine.getCurrentState());
}

void test_emergency_state_ignores_normal_events(void) {
    // Force emergency state
    frontSensor.setMockDistance(5.0f);
    safetyMonitor.checkSafety();
    TEST_ASSERT_EQUAL(StateMachine::STATE_EMERGENCY, stateMachine.getCurrentState());

    // Try various events that should be ignored
    stateMachine.processEvent(StateMachine::EVENT_START_PRESSED);
    TEST_ASSERT_EQUAL(StateMachine::STATE_EMERGENCY, stateMachine.getCurrentState());

    stateMachine.processEvent(StateMachine::EVENT_SPEED_UP);
    TEST_ASSERT_EQUAL(StateMachine::STATE_EMERGENCY, stateMachine.getCurrentState());

    stateMachine.processEvent(StateMachine::EVENT_DOOR_TOGGLE);
    TEST_ASSERT_EQUAL(StateMachine::STATE_EMERGENCY, stateMachine.getCurrentState());
}

void test_emergency_reset(void) {
    // Force emergency state
    frontSensor.setMockDistance(5.0f);
    safetyMonitor.checkSafety();
    TEST_ASSERT_EQUAL(StateMachine::STATE_EMERGENCY, stateMachine.getCurrentState());

    // Send emergency reset event
    stateMachine.processEvent(StateMachine::EVENT_EMERGENCY_RESET);

    // Verify return to idle
    TEST_ASSERT_EQUAL(StateMachine::STATE_IDLE, stateMachine.getCurrentState());
}

void test_door_obstacle_detection(void) {
    // Start door opening
    stateMachine.processEvent(StateMachine::EVENT_DOOR_TOGGLE);
    TEST_ASSERT_EQUAL(StateMachine::STATE_DOOR_OPENING, stateMachine.getCurrentState());

    // Simulate obstacle during door operation
    frontSensor.setMockDistance(5.0f);
    safetyMonitor.checkSafety();

    // Verify transition to ERROR state (not emergency)
    TEST_ASSERT_EQUAL(StateMachine::STATE_ERROR, stateMachine.getCurrentState());
}

void test_ground_detection_filtering(void) {
    // Setup: Start swinging with user present
    pressureSensor.setMockOccupied(true);
    stateMachine.processEvent(StateMachine::EVENT_PRESSURE_ON);
    stateMachine.processEvent(StateMachine::EVENT_START_PRESSED);

    // Simulate ground detection pattern (rapid drop to <40cm from >80cm)
    frontSensor.setMockDistance(100.0f);
    safetyMonitor.checkObstacles(); // First reading

    // Simulate sudden drop in distance (like detecting ground)
    frontSensor.setMockDistance(30.0f);
    SafetyMonitor::SafetyStatus status = safetyMonitor.checkObstacles();

    // Should not report as obstacle due to ground filtering
    TEST_ASSERT_EQUAL(SafetyMonitor::STATUS_OK, status);
    TEST_ASSERT_EQUAL(StateMachine::STATE_SWINGING, stateMachine.getCurrentState());
}

void run_safety_state_integration_tests() {
    UNITY_BEGIN();
    RUN_TEST(test_obstacle_detection_to_emergency_state);
    RUN_TEST(test_user_absence_during_swing);
    RUN_TEST(test_motor_stall_detection);
    RUN_TEST(test_emergency_state_ignores_normal_events);
    RUN_TEST(test_emergency_reset);
    RUN_TEST(test_door_obstacle_detection);
    RUN_TEST(test_ground_detection_filtering);
    UNITY_END();
}

void setup() {
    delay(2000);
    run_safety_state_integration_tests();
}

void loop() {
    // Nothing to do here
}
