// test/system_tests/test_door_operations.cpp
#include <unity.h>
#include "StateMachine.h"
#include "SafetyMonitor.h"
#include "ActuatorDriver.h"
#include "UltrasonicSensor.h"

// Mock classes
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

// Mock millis() function for timeout testing
unsigned long mockMillisValue = 0;
unsigned long millis() {
    return mockMillisValue;
}

// Test fixtures
StateMachine stateMachine;
MockUltrasonicSensor frontSensor("Front");
MockUltrasonicSensor rearSensor("Rear");
ActuatorDriver doorActuator(PIN_ACTUATOR_FWD, PIN_ACTUATOR_REV, &stateMachine);
SafetyMonitor safetyMonitor(&stateMachine, &frontSensor, &rearSensor, nullptr);

void setUp(void) {
    // Reset components before each test
    mockMillisValue = 1000; // Start at 1 second
    stateMachine = StateMachine();
    frontSensor = MockUltrasonicSensor("Front");
    rearSensor = MockUltrasonicSensor("Rear");
    doorActuator = ActuatorDriver(PIN_ACTUATOR_FWD, PIN_ACTUATOR_REV, &stateMachine);
    safetyMonitor.~SafetyMonitor();
    new (&safetyMonitor) SafetyMonitor(&stateMachine, &frontSensor, &rearSensor, nullptr);

    // Initialize components
    stateMachine.setDoorActuator(&doorActuator);
    stateMachine.setDoorTimeout(5000); // 5 seconds timeout
    stateMachine.begin();
    doorActuator.begin();
    safetyMonitor.begin();

    // Set initial sensor values
    frontSensor.setMockDistance(100.0f);
    rearSensor.setMockDistance(100.0f);
}

void tearDown(void) {
    // Cleanup
}

void test_normal_door_opening_sequence(void) {
    // Start door opening
    stateMachine.processEvent(StateMachine::EVENT_DOOR_TOGGLE);

    // Verify state transition
    TEST_ASSERT_EQUAL(StateMachine::STATE_DOOR_OPENING, stateMachine.getCurrentState());
    TEST_ASSERT_EQUAL(ActuatorDriver::DIRECTION_EXTEND, doorActuator.getCurrentDirection());

    // Simulate door fully opened after some time
    mockMillisValue += 4500; // 4.5 seconds (within 5s timeout)
    doorActuator.update();

    // Verify door opened event triggered
    TEST_ASSERT_EQUAL(StateMachine::STATE_IDLE, stateMachine.getCurrentState());
    TEST_ASSERT_EQUAL(ActuatorDriver::DIRECTION_STOP, doorActuator.getCurrentDirection());
}

void test_normal_door_closing_sequence(void) {
    // First open the door
    stateMachine.processEvent(StateMachine::EVENT_DOOR_TOGGLE);
    mockMillisValue += 4500;
    doorActuator.update();

    // Now close the door
    stateMachine.processEvent(StateMachine::EVENT_DOOR_TOGGLE);

    // Verify state transition
    TEST_ASSERT_EQUAL(StateMachine::STATE_DOOR_CLOSING, stateMachine.getCurrentState());
    TEST_ASSERT_EQUAL(ActuatorDriver::DIRECTION_RETRACT, doorActuator.getCurrentDirection());

    // Simulate door fully closed after some time
    mockMillisValue += 4500; // 4.5 seconds (within 5s timeout)
    doorActuator.update();

    // Verify door closed event triggered
    TEST_ASSERT_EQUAL(StateMachine::STATE_IDLE, stateMachine.getCurrentState());
    TEST_ASSERT_EQUAL(ActuatorDriver::DIRECTION_STOP, doorActuator.getCurrentDirection());
}

void test_interrupting_door_operation(void) {
    // Start door opening
    stateMachine.processEvent(StateMachine::EVENT_DOOR_TOGGLE);
    TEST_ASSERT_EQUAL(StateMachine::STATE_DOOR_OPENING, stateMachine.getCurrentState());

    // Interrupt with another toggle before completion
    mockMillisValue += 2000; // 2 seconds
    stateMachine.processEvent(StateMachine::EVENT_DOOR_TOGGLE);

    // Should now be closing
    TEST_ASSERT_EQUAL(StateMachine::STATE_DOOR_CLOSING, stateMachine.getCurrentState());
    TEST_ASSERT_EQUAL(ActuatorDriver::DIRECTION_RETRACT, doorActuator.getCurrentDirection());

    // Interrupt again
    mockMillisValue += 1000; // 1 second
    stateMachine.processEvent(StateMachine::EVENT_DOOR_TOGGLE);

    // Should now be opening again
    TEST_ASSERT_EQUAL(StateMachine::STATE_DOOR_OPENING, stateMachine.getCurrentState());
    TEST_ASSERT_EQUAL(ActuatorDriver::DIRECTION_EXTEND, doorActuator.getCurrentDirection());
}

void test_door_opening_timeout(void) {
    // Start door opening
    stateMachine.processEvent(StateMachine::EVENT_DOOR_TOGGLE);
    TEST_ASSERT_EQUAL(StateMachine::STATE_DOOR_OPENING, stateMachine.getCurrentState());

    // Advance time beyond timeout threshold without completing operation
    mockMillisValue += 6000; // 6 seconds (exceeds 5s timeout)
    stateMachine.update(); // This should check for timeout

    // Verify timeout was detected and handled
    TEST_ASSERT_EQUAL(StateMachine::STATE_ERROR, stateMachine.getCurrentState());
    TEST_ASSERT_EQUAL(ActuatorDriver::DIRECTION_STOP, doorActuator.getCurrentDirection());
}

void test_door_closing_timeout(void) {
    // First open the door
    stateMachine.processEvent(StateMachine::EVENT_DOOR_TOGGLE);
    mockMillisValue += 4500;
    doorActuator.update();

    // Now close the door
    stateMachine.processEvent(StateMachine::EVENT_DOOR_TOGGLE);
    TEST_ASSERT_EQUAL(StateMachine::STATE_DOOR_CLOSING, stateMachine.getCurrentState());

    // Advance time beyond timeout threshold without completing operation
    mockMillisValue += 6000; // 6 seconds (exceeds 5s timeout)
    stateMachine.update(); // This should check for timeout

    // Verify timeout was detected and handled
    TEST_ASSERT_EQUAL(StateMachine::STATE_ERROR, stateMachine.getCurrentState());
    TEST_ASSERT_EQUAL(ActuatorDriver::DIRECTION_STOP, doorActuator.getCurrentDirection());
}

void test_recovering_from_door_timeout(void) {
    // Trigger a timeout
    stateMachine.processEvent(StateMachine::EVENT_DOOR_TOGGLE);
    mockMillisValue += 6000;
    stateMachine.update();
    TEST_ASSERT_EQUAL(StateMachine::STATE_ERROR, stateMachine.getCurrentState());

    // Try to recover
    stateMachine.processEvent(StateMachine::EVENT_ERROR_CLEARED);

    // Verify recovery
    TEST_ASSERT_EQUAL(StateMachine::STATE_IDLE, stateMachine.getCurrentState());

    // Verify system can operate again
    stateMachine.processEvent(StateMachine::EVENT_DOOR_TOGGLE);
    TEST_ASSERT_EQUAL(StateMachine::STATE_DOOR_OPENING, stateMachine.getCurrentState());
}

void test_obstacle_detection_during_opening(void) {
    // Start door opening
    stateMachine.processEvent(StateMachine::EVENT_DOOR_TOGGLE);
    TEST_ASSERT_EQUAL(StateMachine::STATE_DOOR_OPENING, stateMachine.getCurrentState());

    // Simulate obstacle detection (distance < 30cm)
    frontSensor.setMockDistance(25.0f);
    SafetyMonitor::SafetyStatus status = safetyMonitor.checkSafety();

    // Verify obstacle was detected properly
    TEST_ASSERT_EQUAL(SafetyMonitor::STATUS_ERROR, status);
    TEST_ASSERT_EQUAL(StateMachine::STATE_ERROR, stateMachine.getCurrentState());
    TEST_ASSERT_EQUAL(ActuatorDriver::DIRECTION_STOP, doorActuator.getCurrentDirection());
}

void test_obstacle_detection_during_closing(void) {
    // Open door first
    stateMachine.processEvent(StateMachine::EVENT_DOOR_TOGGLE);
    mockMillisValue += 4500;
    doorActuator.update();

    // Start door closing
    stateMachine.processEvent(StateMachine::EVENT_DOOR_TOGGLE);
    TEST_ASSERT_EQUAL(StateMachine::STATE_DOOR_CLOSING, stateMachine.getCurrentState());

    // Simulate critical obstacle detection (distance < 10cm)
    rearSensor.setMockDistance(5.0f);
    SafetyMonitor::SafetyStatus status = safetyMonitor.checkSafety();

    // Verify critical obstacle triggers emergency
    TEST_ASSERT_EQUAL(SafetyMonitor::STATUS_EMERGENCY, status);
    TEST_ASSERT_EQUAL(StateMachine::STATE_EMERGENCY, stateMachine.getCurrentState());
    TEST_ASSERT_EQUAL(ActuatorDriver::DIRECTION_STOP, doorActuator.getCurrentDirection());
}

void test_door_operation_after_obstacle(void) {
    // Trigger obstacle error
    stateMachine.processEvent(StateMachine::EVENT_DOOR_TOGGLE);
    frontSensor.setMockDistance(25.0f);
    safetyMonitor.checkSafety();
    TEST_ASSERT_EQUAL(StateMachine::STATE_ERROR, stateMachine.getCurrentState());

    // Clear error and try door operation again
    frontSensor.setMockDistance(100.0f); // Remove obstacle
    stateMachine.processEvent(StateMachine::EVENT_ERROR_CLEARED);
    TEST_ASSERT_EQUAL(StateMachine::STATE_IDLE, stateMachine.getCurrentState());

    // Door should work again
    stateMachine.processEvent(StateMachine::EVENT_DOOR_TOGGLE);
    TEST_ASSERT_EQUAL(StateMachine::STATE_DOOR_OPENING, stateMachine.getCurrentState());
}

void run_door_operation_tests(void) {
    UNITY_BEGIN();
    RUN_TEST(test_normal_door_opening_sequence);
    RUN_TEST(test_normal_door_closing_sequence);
    RUN_TEST(test_interrupting_door_operation);
    RUN_TEST(test_door_opening_timeout);
    RUN_TEST(test_door_closing_timeout);
    RUN_TEST(test_recovering_from_door_timeout);
    RUN_TEST(test_obstacle_detection_during_opening);
    RUN_TEST(test_obstacle_detection_during_closing);
    RUN_TEST(test_door_operation_after_obstacle);
    UNITY_END();
}

void setup() {
    delay(2000);
    run_door_operation_tests();
}

void loop() {
    // Nothing to do here
}
