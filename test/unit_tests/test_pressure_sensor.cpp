// test/unit_tests/test_pressure_sensor.cpp
#include <unity.h>
#include "PressureSensor.h"
#include "Configuration.h"

// Mock analog read function
namespace {
    int mockAnalogValue = 0;
    int analogReadCalls = 0;

    void resetMocks() {
        mockAnalogValue = 0;
        analogReadCalls = 0;
    }
}

// Override Arduino's analogRead function for testing
int analogRead(uint8_t pin) {
    analogReadCalls++;
    return mockAnalogValue;
}

// Test fixture
const uint8_t TEST_PIN = PIN_PRESSURE_SENSOR;
const int TEST_THRESHOLD = PRESSURE_THRESHOLD;
PressureSensor sensor(TEST_PIN, TEST_THRESHOLD, "TestSensor");

void setUp(void) {
    resetMocks();
    sensor.begin();
}

void tearDown(void) {
    // Cleanup
}

// Test cases

void test_initialization(void) {
    // Verify initialization doesn't change state
    TEST_ASSERT_EQUAL(0, sensor.getLastReading());
    TEST_ASSERT_EQUAL(TEST_THRESHOLD, sensor.getThreshold());
    TEST_ASSERT_EQUAL_STRING("TestSensor", sensor.getName());
}

void test_read_raw_value(void) {
    // Test reading raw analog value
    mockAnalogValue = 300;
    int rawValue = sensor.readRawValue();

    TEST_ASSERT_EQUAL(1, analogReadCalls); // Verify analog read was called
    TEST_ASSERT_EQUAL(300, rawValue);      // Verify correct value returned
    TEST_ASSERT_EQUAL(300, sensor.getLastReading()); // Verify last reading updated
}

void test_occupancy_detection_below_threshold(void) {
    // Test when pressure is below threshold
    mockAnalogValue = TEST_THRESHOLD - 1;
    bool occupied = sensor.isOccupied();

    TEST_ASSERT_FALSE(occupied);
    TEST_ASSERT_EQUAL(TEST_THRESHOLD - 1, sensor.getLastReading());
}

void test_occupancy_detection_at_threshold(void) {
    // Test when pressure is exactly at threshold
    mockAnalogValue = TEST_THRESHOLD;
    bool occupied = sensor.isOccupied();

    TEST_ASSERT_FALSE(occupied); // Should be false at exactly threshold
}

void test_occupancy_detection_above_threshold(void) {
    // Test when pressure is above threshold
    mockAnalogValue = TEST_THRESHOLD + 1;
    bool occupied = sensor.isOccupied();

    TEST_ASSERT_TRUE(occupied);
    TEST_ASSERT_EQUAL(TEST_THRESHOLD + 1, sensor.getLastReading());
}

void test_threshold_behavior(void) {
    // Test sequence of values around threshold
    for (int value = TEST_THRESHOLD - 10; value <= TEST_THRESHOLD + 10; value++) {
        mockAnalogValue = value;
        bool occupied = sensor.isOccupied();

        if (value > TEST_THRESHOLD) {
            TEST_ASSERT_TRUE(occupied);
        } else {
            TEST_ASSERT_FALSE(occupied);
        }
    }
}

void test_analog_range_limits(void) {
    // Test minimum analog reading (0)
    mockAnalogValue = 0;
    TEST_ASSERT_FALSE(sensor.isOccupied());

    // Test maximum analog reading (1023)
    mockAnalogValue = 1023;
    TEST_ASSERT_TRUE(sensor.isOccupied());
}

void test_reading_maintains_state(void) {
    // Verify that last reading is maintained after readRawValue
    mockAnalogValue = 700;
    sensor.readRawValue();

    // Change mock value but don't read again
    mockAnalogValue = 300;

    TEST_ASSERT_EQUAL(700, sensor.getLastReading());
}

void test_custom_threshold(void) {
    // Create sensor with custom threshold
    PressureSensor customSensor(TEST_PIN, 700, "CustomThreshold");
    customSensor.begin();

    // Test with value below custom threshold
    mockAnalogValue = 699;
    TEST_ASSERT_FALSE(customSensor.isOccupied());

    // Test with value above custom threshold
    mockAnalogValue = 701;
    TEST_ASSERT_TRUE(customSensor.isOccupied());

    TEST_ASSERT_EQUAL(700, customSensor.getThreshold());
}

void run_pressure_sensor_tests() {
    UNITY_BEGIN();
    RUN_TEST(test_initialization);
    RUN_TEST(test_read_raw_value);
    RUN_TEST(test_occupancy_detection_below_threshold);
    RUN_TEST(test_occupancy_detection_at_threshold);
    RUN_TEST(test_occupancy_detection_above_threshold);
    RUN_TEST(test_threshold_behavior);
    RUN_TEST(test_analog_range_limits);
    RUN_TEST(test_reading_maintains_state);
    RUN_TEST(test_custom_threshold);
    UNITY_END();
}

void setup() {
    delay(2000);
    run_pressure_sensor_tests();
}

void loop() {
    // Nothing to do here
}
