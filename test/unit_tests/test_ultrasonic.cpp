// test/unit_tests/test_ultrasonic_sensor.cpp
#include <unity.h>
#include "UltrasonicSensor.h"
#include "Configuration.h"

// Mock Arduino functions
namespace {
    unsigned long mockPulseInValue = 0;
    int mockPinStates[14] = {0}; // Store digital pin states
    unsigned long mockMicrosValue = 0;
    bool mockEchoState = false;

    void resetMocks() {
        mockPulseInValue = 0;
        mockMicrosValue = 0;
        mockEchoState = false;
        for (int i = 0; i < 14; i++) {
            mockPinStates[i] = 0;
        }
    }
}

// Mock pulseIn function
unsigned long pulseIn(uint8_t pin, uint8_t state, unsigned long timeout) {
    return mockPulseInValue;
}

// Mock digitalWrite function
void digitalWrite(uint8_t pin, uint8_t val) {
    if (pin < 14) {
        mockPinStates[pin] = val;
    }
}

// Mock digitalRead function
int mockDigitalRead(uint8_t pin) {
    if (pin == PIN_ULTRASONIC1_ECHO || pin == PIN_ULTRASONIC2_ECHO) {
        return mockEchoState ? HIGH : LOW;
    }
    return LOW;
}

#define digitalRead(pin) mockDigitalRead(pin)

// Mock micros function
unsigned long micros() {
    return mockMicrosValue;
}

// Test fixture
const uint8_t TEST_TRIG_PIN = PIN_ULTRASONIC1_TRIG;
const uint8_t TEST_ECHO_PIN = PIN_ULTRASONIC1_ECHO;
UltrasonicSensor sensor(TEST_TRIG_PIN, TEST_ECHO_PIN, "TestSensor");

void setUp(void) {
    resetMocks();
    sensor.begin();
}

void tearDown(void) {
    // Cleanup
}

// Helper function to convert distance to pulse duration
unsigned long distanceToDuration(float distanceCm) {
    // d = (t * 0.034) / 2
    // t = d * 2 / 0.034
    return (unsigned long)(distanceCm * 2 / 0.034);
}

// Test cases
void test_initialization(void) {
    // Re-initialize to verify pin setup
    sensor.begin();

    // Verify trig pin set as output and initialized to LOW
    TEST_ASSERT_EQUAL(LOW, mockPinStates[TEST_TRIG_PIN]);
}

void test_distance_measurement_valid(void) {
    // Test with a known distance (50cm)
    float expectedDistance = 50.0f;
    mockPulseInValue = distanceToDuration(expectedDistance);

    float measuredDistance = sensor.measureDistance();

    // Allow small floating-point precision error (0.1cm tolerance)
    TEST_ASSERT_FLOAT_WITHIN(0.1f, expectedDistance, measuredDistance);
}

void test_distance_measurement_timeout(void) {
    // Test timeout condition (no echo received)
    mockPulseInValue = 0; // No echo received

    float measuredDistance = sensor.measureDistance();

    // Should return 0 for timeout/no echo
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 0.0f, measuredDistance);
}

void test_distance_measurement_max_range(void) {
    // Test maximum range (~400cm for HC-SR04)
    float expectedDistance = 400.0f;
    mockPulseInValue = distanceToDuration(expectedDistance);

    float measuredDistance = sensor.measureDistance();

    TEST_ASSERT_FLOAT_WITHIN(0.1f, expectedDistance, measuredDistance);
}

void test_obstacle_warning_threshold(void) {
    // Test distance just above warning threshold
    float safeDistance = OBSTACLE_DISTANCE_CM + 1.0f;
    mockPulseInValue = distanceToDuration(safeDistance);
    float measuredDistance = sensor.measureDistance();
    TEST_ASSERT_TRUE(measuredDistance > OBSTACLE_DISTANCE_CM);

    // Test distance just below warning threshold
    float warningDistance = OBSTACLE_DISTANCE_CM - 1.0f;
    mockPulseInValue = distanceToDuration(warningDistance);
    measuredDistance = sensor.measureDistance();
    TEST_ASSERT_TRUE(measuredDistance < OBSTACLE_DISTANCE_CM);
}

void test_critical_distance_threshold(void) {
    // Test distance just above critical threshold
    float warningDistance = CRITICAL_DISTANCE_CM + 1.0f;
    mockPulseInValue = distanceToDuration(warningDistance);
    float measuredDistance = sensor.measureDistance();
    TEST_ASSERT_TRUE(measuredDistance > CRITICAL_DISTANCE_CM);

    // Test distance just below critical threshold
    float criticalDistance = CRITICAL_DISTANCE_CM - 1.0f;
    mockPulseInValue = distanceToDuration(criticalDistance);
    measuredDistance = sensor.measureDistance();
    TEST_ASSERT_TRUE(measuredDistance < CRITICAL_DISTANCE_CM);
}

void test_last_distance_storage(void) {
    // Verify getLastDistance returns the last measurement
    float expectedDistance = 75.0f;
    mockPulseInValue = distanceToDuration(expectedDistance);

    sensor.measureDistance();
    float storedDistance = sensor.getLastDistance();

    TEST_ASSERT_FLOAT_WITHIN(0.1f, expectedDistance, storedDistance);
}

void test_non_blocking_measurement(void) {
    // Start a non-blocking measurement
    mockMicrosValue = 1000; // Starting time
    sensor.startMeasurement();

    // Should not be complete yet
    mockEchoState = true; // Echo pin is HIGH
    mockMicrosValue = 1500; // Time during measurement
    TEST_ASSERT_FALSE(sensor.isMeasurementComplete());

    // Complete the measurement
    mockEchoState = false; // Echo pin is now LOW
    mockMicrosValue = 2000; // Time after echo received

    // Calculate expected distance (time = 1000 microseconds)
    // distance = (time * 0.034) / 2 = (1000 * 0.034) / 2 = 17cm
    float expectedDistance = 17.0f;

    TEST_ASSERT_TRUE(sensor.isMeasurementComplete());
    TEST_ASSERT_FLOAT_WITHIN(0.1f, expectedDistance, sensor.getLastDistance());
}

void test_non_blocking_timeout(void) {
    // Start a non-blocking measurement
    mockMicrosValue = 1000; // Starting time
    sensor.startMeasurement();

    // Simulate a timeout (no echo received within timeout period)
    mockEchoState = true; // Echo pin stays HIGH
    mockMicrosValue = 1000 + UltrasonicSensor::MEASUREMENT_TIMEOUT + 100; // Time after timeout

    TEST_ASSERT_TRUE(sensor.isMeasurementComplete());
    TEST_ASSERT_EQUAL(-1.0f, sensor.getLastDistance()); // Should indicate invalid distance
}

void test_name_retrieval(void) {
    // Verify getName returns the correct sensor name
    TEST_ASSERT_EQUAL_STRING("TestSensor", sensor.getName());
}

void run_ultrasonic_tests(void) {
    UNITY_BEGIN();
    RUN_TEST(test_initialization);
    RUN_TEST(test_distance_measurement_valid);
    RUN_TEST(test_distance_measurement_timeout);
    RUN_TEST(test_distance_measurement_max_range);
    RUN_TEST(test_obstacle_warning_threshold);
    RUN_TEST(test_critical_distance_threshold);
    RUN_TEST(test_last_distance_storage);
    RUN_TEST(test_non_blocking_measurement);
    RUN_TEST(test_non_blocking_timeout);
    RUN_TEST(test_name_retrieval);
    UNITY_END();
}

void setup() {
    delay(2000);
    run_ultrasonic_tests();
}

void loop() {
    // Nothing to do here
}
