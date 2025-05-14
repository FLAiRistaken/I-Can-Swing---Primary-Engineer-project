// test/unit_tests/test_safety_monitor.cpp
#include <unity.h>
#include "SafetyMonitor.h"
#include "StateMachine.h"
#include "UltrasonicSensor.h"
#include "PressureSensor.h"

// Mock classes for dependencies
class MockStateMachine : public StateMachine {
public:
    MockStateMachine() : _state(STATE_IDLE), _lastEvent(EVENT_NONE) {}

    void processEvent(Event event) override {
        _lastEvent = event;
        if (event == EVENT_EMERGENCY) {
            _state = STATE_EMERGENCY;
        } else if (event == EVENT_OBSTACLE_DETECTED) {
            _state = STATE_ERROR;
        } else if (event == EVENT_START_PRESSED && _mockUserPresent) {
            _state = STATE_SWINGING;
        } else if (event == EVENT_STOP_PRESSED) {
            _state = STATE_IDLE;
        }
    }

    State getCurrentState() const override {
        return _state;
    }

    Speed getCurrentSpeed() const override {
        return _speed;
    }

    void setMockState(State state) {
        _state = state;
    }

    void setMockSpeed(Speed speed) {
        _speed = speed;
    }

    void setMockUserPresent(bool present) {
        _mockUserPresent = present;
    }

    Event getLastEvent() const {
        return _lastEvent;
    }

private:
    State _state;
    Speed _speed = SPEED_OFF;
    Event _lastEvent;
    bool _mockUserPresent = false;
};

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
        : PressureSensor(0, 0, name), _mockOccupied(false), _mockReading(0) {}

    bool isOccupied() override {
        return _mockOccupied;
    }

    int readRawValue() override {
        return _mockReading;
    }

    void setMockOccupied(bool occupied) {
        _mockOccupied = occupied;
    }

    void setMockReading(int reading) {
        _mockReading = reading;
    }

private:
    bool _mockOccupied;
    int _mockReading;
};

// Test fixtures
MockStateMachine mockStateMachine;
MockUltrasonicSensor mockFrontSensor("FrontMock");
MockUltrasonicSensor mockRearSensor("RearMock");
MockPressureSensor mockPressureSensor("PressureMock");
SafetyMonitor safetyMonitor(&mockStateMachine, &mockFrontSensor, &mockRearSensor, &mockPressureSensor);

void setUp(void) {
    // Reset mock objects before each test
    mockStateMachine.setMockState(StateMachine::STATE_IDLE);
    mockStateMachine.setMockSpeed(StateMachine::SPEED_OFF);
    mockFrontSensor.setMockDistance(100.0f);
    mockRearSensor.setMockDistance(100.0f);
    mockPressureSensor.setMockOccupied(false);
    mockPressureSensor.setMockReading(0);

    // Initialize safety monitor
    safetyMonitor.begin();
}

void tearDown(void) {
    // Cleanup after each test
}

// Test cases
void test_initial_safety_status(void) {
    // Should be OK when no hazards present
    SafetyMonitor::SafetyStatus status = safetyMonitor.checkSafety();
    TEST_ASSERT_EQUAL(SafetyMonitor::STATUS_OK, status);
    TEST_ASSERT_EQUAL(SafetyMonitor::STATUS_OK, safetyMonitor.getCurrentStatus());
}

void test_obstacle_detection_front(void) {
    // Test warning distance
    mockFrontSensor.setMockDistance(25.0f); // Within warning range
    SafetyMonitor::SafetyStatus status = safetyMonitor.checkObstacles();
    TEST_ASSERT_EQUAL(SafetyMonitor::STATUS_ERROR, status);

    // Test critical distance
    mockFrontSensor.setMockDistance(5.0f); // Within critical range
    status = safetyMonitor.checkObstacles();
    TEST_ASSERT_EQUAL(SafetyMonitor::STATUS_EMERGENCY, status);
}

void test_obstacle_detection_rear(void) {
    // Test warning distance
    mockRearSensor.setMockDistance(25.0f); // Within warning range
    SafetyMonitor::SafetyStatus status = safetyMonitor.checkObstacles();
    TEST_ASSERT_EQUAL(SafetyMonitor::STATUS_ERROR, status);

    // Test critical distance
    mockRearSensor.setMockDistance(5.0f); // Within critical range
    status = safetyMonitor.checkObstacles();
    TEST_ASSERT_EQUAL(SafetyMonitor::STATUS_EMERGENCY, status);
}

void test_user_presence_detection(void) {
    // When idle, user absence should be OK
    mockPressureSensor.setMockOccupied(false);
    mockStateMachine.setMockState(StateMachine::STATE_IDLE);
    SafetyMonitor::SafetyStatus status = safetyMonitor.checkUserPresence();
    TEST_ASSERT_EQUAL(SafetyMonitor::STATUS_OK, status);

    // When swinging, user absence should trigger emergency
    mockPressureSensor.setMockOccupied(false);
    mockStateMachine.setMockState(StateMachine::STATE_SWINGING);
    status = safetyMonitor.checkUserPresence();
    TEST_ASSERT_EQUAL(SafetyMonitor::STATUS_EMERGENCY, status);

    // User presence during swinging should be OK
    mockPressureSensor.setMockOccupied(true);
    status = safetyMonitor.checkUserPresence();
    TEST_ASSERT_EQUAL(SafetyMonitor::STATUS_OK, status);
}

void test_motor_stall_detection(void) {
    // Setup: motors active but stalled
    safetyMonitor.updateMotorStatus(true, 500); // Initial speed
    SafetyMonitor::SafetyStatus status = safetyMonitor.checkMotorOperation();
    TEST_ASSERT_EQUAL(SafetyMonitor::STATUS_OK, status);

    // Simulate motor stall (active but zero speed)
    safetyMonitor.updateMotorStatus(true, 0);
    status = safetyMonitor.checkMotorOperation();
    TEST_ASSERT_EQUAL(SafetyMonitor::STATUS_OK, status); // First detection

    // Multiple stall detections should trigger emergency
    safetyMonitor.updateMotorStatus(true, 0);
    safetyMonitor.updateMotorStatus(true, 0);
    status = safetyMonitor.checkMotorOperation();
    TEST_ASSERT_EQUAL(SafetyMonitor::STATUS_EMERGENCY, status);
}

void test_ground_detection_filtering(void) {
    // Setup: swinging state
    mockStateMachine.setMockState(StateMachine::STATE_SWINGING);

    // Simulate approach toward ground
    mockFrontSensor.setMockDistance(100.0f);
    safetyMonitor.checkObstacles(); // First reading

    // Simulate sudden drop in distance (like detecting ground)
    mockFrontSensor.setMockDistance(30.0f);
    SafetyMonitor::SafetyStatus status = safetyMonitor.checkObstacles();

    // Should not report as obstacle due to filtering
    TEST_ASSERT_EQUAL(SafetyMonitor::STATUS_OK, status);
}

void test_dynamic_thresholds_during_swing(void) {
    // When not swinging, threshold should be normal
    mockStateMachine.setMockState(StateMachine::STATE_IDLE);
    mockFrontSensor.setMockDistance(25.0f); // Between adjusted and normal thresholds
    SafetyMonitor::SafetyStatus status = safetyMonitor.checkObstacles();
    TEST_ASSERT_EQUAL(SafetyMonitor::STATUS_ERROR, status);

    // When swinging, threshold should be reduced
    mockStateMachine.setMockState(StateMachine::STATE_SWINGING);
    mockFrontSensor.setMockDistance(25.0f); // This should now be OK with adjusted threshold
    status = safetyMonitor.checkObstacles();
    // Exact behavior depends on threshold values in your code
}

void test_watchdog_functionality(void) {
    // This is a bit tricky to test as it depends on time
    // One approach is to test the check method itself, rather than time passage
    SafetyMonitor::SafetyStatus status = safetyMonitor.checkSystemHealth();
    TEST_ASSERT_EQUAL(SafetyMonitor::STATUS_OK, status);

    // Would need to manipulate internal watchdog timer to really test timeout
}

void test_safety_status_transition_to_state_machine(void) {
    // Simulate obstacle detection
    mockFrontSensor.setMockDistance(5.0f); // Critical distance

    // Check safety - should trigger emergency
    safetyMonitor.checkSafety();

    // Verify that an emergency event was sent to state machine
    TEST_ASSERT_EQUAL(StateMachine::EVENT_EMERGENCY, mockStateMachine.getLastEvent());
}

void test_integration_of_multiple_safety_checks(void) {
    // Set up multiple safety issues
    mockFrontSensor.setMockDistance(25.0f); // Warning distance (ERROR)
    mockPressureSensor.setMockOccupied(false);
    mockStateMachine.setMockState(StateMachine::STATE_SWINGING); // User absence during swing (EMERGENCY)

    // Emergency should take precedence
    SafetyMonitor::SafetyStatus status = safetyMonitor.checkSafety();
    TEST_ASSERT_EQUAL(SafetyMonitor::STATUS_EMERGENCY, status);
}

void run_safety_monitor_tests() {
    UNITY_BEGIN();
    RUN_TEST(test_initial_safety_status);
    RUN_TEST(test_obstacle_detection_front);
    RUN_TEST(test_obstacle_detection_rear);
    RUN_TEST(test_user_presence_detection);
    RUN_TEST(test_motor_stall_detection);
    RUN_TEST(test_ground_detection_filtering);
    RUN_TEST(test_dynamic_thresholds_during_swing);
    RUN_TEST(test_watchdog_functionality);
    RUN_TEST(test_safety_status_transition_to_state_machine);
    RUN_TEST(test_integration_of_multiple_safety_checks);
    UNITY_END();
}

void setup() {
    delay(2000);
    run_safety_monitor_tests();
}

void loop() {
    // Nothing to do here
}
