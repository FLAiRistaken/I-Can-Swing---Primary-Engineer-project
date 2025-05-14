// test/unit_tests/test_actuator_driver.cpp
#include <unity.h>
#include "ActuatorDriver.h"
#include "StateMachine.h"

// Mock Arduino functions
namespace {
    int mockPinStates[14] = {0};
    unsigned long mockMillis = 0;

    void resetMocks() {
        for (int i = 0; i < 14; i++) {
            mockPinStates[i] = 0;
        }
        mockMillis = 0;
    }
}

// Override Arduino's digitalWrite function
void digitalWrite(uint8_t pin, uint8_t val) {
    if (pin < 14) {
        mockPinStates[pin] = val;
    }
}

// Override Arduino's pinMode function
void pinMode(uint8_t pin, uint8_t mode) {
    // Just a stub for testing
}

// Override Arduino's millis function
unsigned long millis() {
    return mockMillis;
}

// Mock StateMachine to capture events
class MockStateMachine : public StateMachine {
public:
    MockStateMachine() : _lastEvent(EVENT_NONE), _eventCount(0) {}

    void processEvent(Event event) override {
        _lastEvent = event;
        _eventHistory[_eventCount % 10] = event;
        _eventCount++;
    }

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

// Test fixture
MockStateMachine mockStateMachine;
ActuatorDriver actuator(PIN_ACTUATOR_FWD, PIN_ACTUATOR_REV, &mockStateMachine);

void setUp(void) {
    resetMocks();
    mockStateMachine.reset();
    actuator = ActuatorDriver(PIN_ACTUATOR_FWD, PIN_ACTUATOR_REV, &mockStateMachine);
    actuator.begin();
}

void tearDown(void) {
    // Cleanup
}

// Test cases
void test_initialization() {
    // Verify pins are set to LOW initially
    TEST_ASSERT_EQUAL(LOW, mockPinStates[PIN_ACTUATOR_FWD]);
    TEST_ASSERT_EQUAL(LOW, mockPinStates[PIN_ACTUATOR_REV]);

    // Verify initial state
    TEST_ASSERT_EQUAL(ActuatorDriver::DIRECTION_STOP, actuator.getCurrentDirection());
    TEST_ASSERT_FALSE(actuator.isMoving());
}

void test_extend_command() {
    // Issue extend command
    actuator.extend();

    // Verify pin states
    TEST_ASSERT_EQUAL(HIGH, mockPinStates[PIN_ACTUATOR_FWD]);
    TEST_ASSERT_EQUAL(LOW, mockPinStates[PIN_ACTUATOR_REV]);

    // Verify state
    TEST_ASSERT_EQUAL(ActuatorDriver::DIRECTION_EXTEND, actuator.getCurrentDirection());
    TEST_ASSERT_TRUE(actuator.isMoving());
}

void test_retract_command() {
    // Issue retract command
    actuator.retract();

    // Verify pin states
    TEST_ASSERT_EQUAL(LOW, mockPinStates[PIN_ACTUATOR_FWD]);
    TEST_ASSERT_EQUAL(HIGH, mockPinStates[PIN_ACTUATOR_REV]);

    // Verify state
    TEST_ASSERT_EQUAL(ActuatorDriver::DIRECTION_RETRACT, actuator.getCurrentDirection());
    TEST_ASSERT_TRUE(actuator.isMoving());
}

void test_stop_command() {
    // First extend, then stop
    actuator.extend();
    actuator.stop();

    // Verify pin states
    TEST_ASSERT_EQUAL(LOW, mockPinStates[PIN_ACTUATOR_FWD]);
    TEST_ASSERT_EQUAL(LOW, mockPinStates[PIN_ACTUATOR_REV]);

    // Verify state
    TEST_ASSERT_EQUAL(ActuatorDriver::DIRECTION_STOP, actuator.getCurrentDirection());
    TEST_ASSERT_FALSE(actuator.isMoving());
}

void test_timed_extend() {
    // Start timed extend (2000ms)
    actuator.startExtend(2000);

    // Verify initial state
    TEST_ASSERT_EQUAL(ActuatorDriver::DIRECTION_EXTEND, actuator.getCurrentDirection());
    TEST_ASSERT_TRUE(actuator.isMoving());

    // Update before timeout - should still be extending
    mockMillis = 1000;  // 1 second later
    actuator.update();
    TEST_ASSERT_EQUAL(ActuatorDriver::DIRECTION_EXTEND, actuator.getCurrentDirection());

    // Update after timeout - should stop and signal state machine
    mockMillis = 3000;  // 3 seconds later (past 2000ms threshold)
    actuator.update();

    // Verify stopped and event sent
    TEST_ASSERT_EQUAL(ActuatorDriver::DIRECTION_STOP, actuator.getCurrentDirection());
    TEST_ASSERT_FALSE(actuator.isMoving());
    TEST_ASSERT_EQUAL(StateMachine::EVENT_DOOR_OPENED, mockStateMachine.getLastEvent());
}

void test_timed_retract() {
    // Start timed retract (2000ms)
    actuator.startRetract(2000);

    // Verify initial state
    TEST_ASSERT_EQUAL(ActuatorDriver::DIRECTION_RETRACT, actuator.getCurrentDirection());

    // Update after timeout
    mockMillis = 3000;
    actuator.update();

    // Verify stopped and event sent
    TEST_ASSERT_EQUAL(ActuatorDriver::DIRECTION_STOP, actuator.getCurrentDirection());
    TEST_ASSERT_EQUAL(StateMachine::EVENT_DOOR_CLOSED, mockStateMachine.getLastEvent());
}

void test_stop_during_timed_operation() {
    // Start timed extend
    actuator.startExtend(2000);

    // Stop manually before timeout
    mockMillis = 1000;
    actuator.stop();

    // Verify stopped immediately
    TEST_ASSERT_EQUAL(ActuatorDriver::DIRECTION_STOP, actuator.getCurrentDirection());
    TEST_ASSERT_FALSE(actuator.isMoving());

    // Update after what would have been timeout
    mockMillis = 3000;
    actuator.update();

    // Verify no event sent (operation was manually stopped)
    TEST_ASSERT_EQUAL(0, mockStateMachine.getEventCount());
}

void test_direction_change() {
    // Start extending
    actuator.extend();
    TEST_ASSERT_EQUAL(ActuatorDriver::DIRECTION_EXTEND, actuator.getCurrentDirection());

    // Change to retracting
    actuator.retract();
    TEST_ASSERT_EQUAL(ActuatorDriver::DIRECTION_RETRACT, actuator.getCurrentDirection());
    TEST_ASSERT_EQUAL(LOW, mockPinStates[PIN_ACTUATOR_FWD]);
    TEST_ASSERT_EQUAL(HIGH, mockPinStates[PIN_ACTUATOR_REV]);
}

void test_default_timed_operation() {
    // Start timed extend with default time (DOOR_OPEN_TIME_MS = 5000)
    actuator.startExtend();

    // Update before default timeout
    mockMillis = 4000;
    actuator.update();
    TEST_ASSERT_EQUAL(ActuatorDriver::DIRECTION_EXTEND, actuator.getCurrentDirection());

    // Update after default timeout
    mockMillis = 6000;
    actuator.update();
    TEST_ASSERT_EQUAL(ActuatorDriver::DIRECTION_STOP, actuator.getCurrentDirection());
    TEST_ASSERT_EQUAL(StateMachine::EVENT_DOOR_OPENED, mockStateMachine.getLastEvent());
}

void run_actuator_driver_tests() {
    UNITY_BEGIN();
    RUN_TEST(test_initialization);
    RUN_TEST(test_extend_command);
    RUN_TEST(test_retract_command);
    RUN_TEST(test_stop_command);
    RUN_TEST(test_timed_extend);
    RUN_TEST(test_timed_retract);
    RUN_TEST(test_stop_during_timed_operation);
    RUN_TEST(test_direction_change);
    RUN_TEST(test_default_timed_operation);
    UNITY_END();
}

void setup() {
    delay(2000);
    run_actuator_driver_tests();
}

void loop() {
    // Nothing to do here
}
