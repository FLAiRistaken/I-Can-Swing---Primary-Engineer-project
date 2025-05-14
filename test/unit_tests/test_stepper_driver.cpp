// test/unit_tests/test_stepper_driver.cpp
#include <unity.h>
#include "StepperDriver.h"
#include "Configuration.h"

// Mock Arduino functions
namespace {
    // Track digital pin states
    int mockPinStates[14] = {0};

    // Track calls to the Stepper library
    struct StepperCall {
        int numSteps;
        uint16_t speed;
    };

    StepperCall stepperCalls[10] = {};
    int stepperCallCount = 0;
    int stepperStepCalls = 0;
    int stepperSetSpeedCalls = 0;
    uint16_t lastSpeedSet = 0;

    unsigned long mockMicros = 0;

    void resetMocks() {
        for (int i = 0; i < 14; i++) {
            mockPinStates[i] = 0;
        }

        for (int i = 0; i < 10; i++) {
            stepperCalls[i] = {0, 0};
        }

        stepperCallCount = 0;
        stepperStepCalls = 0;
        stepperSetSpeedCalls = 0;
        lastSpeedSet = 0;
        mockMicros = 0;
    }
}

// Mock digital/analog Arduino functions
void digitalWrite(uint8_t pin, uint8_t val) {
    if (pin < 14) {
        mockPinStates[pin] = val;
    }
}

void pinMode(uint8_t pin, uint8_t mode) {
    // Just track that it was called
}

unsigned long micros() {
    return mockMicros;
}

// Mock Stepper class
class Stepper {
public:
    Stepper(int stepsPerRev, int pin1, int pin2, int pin3, int pin4)
      : _stepsPerRev(stepsPerRev), _pin1(pin1), _pin2(pin2), _pin3(pin3), _pin4(pin4) {}

    void setSpeed(uint16_t rpm) {
        lastSpeedSet = rpm;
        stepperSetSpeedCalls++;
    }

    void step(int steps) {
        if (stepperCallCount < 10) {
            stepperCalls[stepperCallCount].numSteps = steps;
            stepperCalls[stepperCallCount].speed = lastSpeedSet;
            stepperCallCount++;
        }
        stepperStepCalls++;
    }

private:
    int _stepsPerRev;
    int _pin1, _pin2, _pin3, _pin4;
};

// Test fixture
void setUp(void) {
    resetMocks();
}

void tearDown(void) {
    // Cleanup
}

// Test cases
void test_initialization(void) {
    // Create driver with test pins
    StepperDriver driver(PIN_STEPPER1_IN1, PIN_STEPPER1_IN2, PIN_STEPPER1_IN3, PIN_STEPPER1_IN4);
    driver.begin();

    // Verify all pins set to LOW initially
    TEST_ASSERT_EQUAL(LOW, mockPinStates[PIN_STEPPER1_IN1]);
    TEST_ASSERT_EQUAL(LOW, mockPinStates[PIN_STEPPER1_IN2]);
    TEST_ASSERT_EQUAL(LOW, mockPinStates[PIN_STEPPER1_IN3]);
    TEST_ASSERT_EQUAL(LOW, mockPinStates[PIN_STEPPER1_IN4]);

    // Verify setSpeed was called with default 60 RPM
    TEST_ASSERT_EQUAL(1, stepperSetSpeedCalls);
    TEST_ASSERT_EQUAL(60, lastSpeedSet);
}

void test_speed_control(void) {
    StepperDriver driver(PIN_STEPPER1_IN1, PIN_STEPPER1_IN2, PIN_STEPPER1_IN3, PIN_STEPPER1_IN4);
    driver.begin();

    resetMocks(); // Clear initialization calls

    // Test setting various speeds
    driver.setSpeed(100);
    TEST_ASSERT_EQUAL(100, lastSpeedSet);

    driver.setSpeed(200);
    TEST_ASSERT_EQUAL(200, lastSpeedSet);

    // Verify total calls
    TEST_ASSERT_EQUAL(2, stepperSetSpeedCalls);
}

void test_direct_stepping(void) {
    StepperDriver driver(PIN_STEPPER1_IN1, PIN_STEPPER1_IN2, PIN_STEPPER1_IN3, PIN_STEPPER1_IN4);
    driver.begin();
    driver.enable(); // Must enable first

    resetMocks(); // Clear initialization calls

    // Test direct stepping
    driver.step(100); // 100 steps forward
    TEST_ASSERT_EQUAL(1, stepperStepCalls);
    TEST_ASSERT_EQUAL(100, stepperCalls[0].numSteps);

    driver.step(-50); // 50 steps backward
    TEST_ASSERT_EQUAL(2, stepperStepCalls);
    TEST_ASSERT_EQUAL(-50, stepperCalls[1].numSteps);
}

void test_enable_disable(void) {
    StepperDriver driver(PIN_STEPPER1_IN1, PIN_STEPPER1_IN2, PIN_STEPPER1_IN3, PIN_STEPPER1_IN4);
    driver.begin();

    resetMocks();

    // Initially disabled, should not step
    driver.step(100);
    TEST_ASSERT_EQUAL(0, stepperStepCalls);

    // Enable and step
    driver.enable();
    driver.step(100);
    TEST_ASSERT_EQUAL(1, stepperStepCalls);

    // Disable and try to step
    driver.disable();
    driver.step(100);
    TEST_ASSERT_EQUAL(1, stepperStepCalls); // Count shouldn't change

    // Verify pins set low on disable
    TEST_ASSERT_EQUAL(LOW, mockPinStates[PIN_STEPPER1_IN1]);
    TEST_ASSERT_EQUAL(LOW, mockPinStates[PIN_STEPPER1_IN2]);
    TEST_ASSERT_EQUAL(LOW, mockPinStates[PIN_STEPPER1_IN3]);
    TEST_ASSERT_EQUAL(LOW, mockPinStates[PIN_STEPPER1_IN4]);
}

void test_continuous_rotation(void) {
    StepperDriver driver(PIN_STEPPER1_IN1, PIN_STEPPER1_IN2, PIN_STEPPER1_IN3, PIN_STEPPER1_IN4);
    driver.begin();
    driver.enable();
    driver.setSpeed(60); // 60 RPM = 1 revolution per second

    resetMocks();

    // Start continuous clockwise rotation
    driver.setDirection(true);
    driver.startContinuous();

    // Verify running flag
    TEST_ASSERT_TRUE(driver.isRunning());

    // Setup timing for update: 200-step motor at 60 RPM = 3.33 steps per second
    // So about 300ms per step
    mockMicros = 1000000; // 1 second
    driver.update();

    // Should make one step
    TEST_ASSERT_EQUAL(1, stepperStepCalls);
    TEST_ASSERT_EQUAL(1, stepperCalls[0].numSteps); // Clockwise = positive

    // Another update without time passing shouldn't step
    driver.update();
    TEST_ASSERT_EQUAL(1, stepperStepCalls);

    // Advance time and update again
    mockMicros += 300000; // 300ms
    driver.update();
    TEST_ASSERT_EQUAL(2, stepperStepCalls);

    // Stop rotation
    driver.stop();
    TEST_ASSERT_FALSE(driver.isRunning());

    // Update shouldn't step after stop
    mockMicros += 300000;
    driver.update();
    TEST_ASSERT_EQUAL(2, stepperStepCalls);
}

void test_direction_control(void) {
    StepperDriver driver(PIN_STEPPER1_IN1, PIN_STEPPER1_IN2, PIN_STEPPER1_IN3, PIN_STEPPER1_IN4);
    driver.begin();
    driver.enable();

    resetMocks();

    // Set to clockwise, make continuous and update
    driver.setDirection(true);
    driver.startContinuous();

    mockMicros = 1000000;
    driver.update();
    TEST_ASSERT_EQUAL(1, stepperCalls[0].numSteps); // Positive = clockwise

    driver.stop();

    // Change direction to counter-clockwise
    driver.setDirection(false);
    driver.startContinuous();

    // Reset mocks and update
    resetMocks();
    mockMicros = 1000000;
    driver.update();
    TEST_ASSERT_EQUAL(-1, stepperCalls[0].numSteps); // Negative = counter-clockwise
}

void run_stepper_driver_tests() {
    UNITY_BEGIN();
    RUN_TEST(test_initialization);
    RUN_TEST(test_speed_control);
    RUN_TEST(test_direct_stepping);
    RUN_TEST(test_enable_disable);
    RUN_TEST(test_continuous_rotation);
    RUN_TEST(test_direction_control);
    UNITY_END();
}

void setup() {
    delay(2000);
    run_stepper_driver_tests();
}

void loop() {
    // Nothing to do here
}
