// test/main_test.cpp
#include <Arduino.h>
#include <unity.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include "Configuration.h"
#include "test_utils/component_detector.h"

// Forward declarations of test groups
void run_state_machine_tests(void);
void run_safety_monitor_tests(void);
void run_ultrasonic_tests(void);
void run_pressure_sensor_tests(void);
void run_stepper_driver_tests(void);
void run_actuator_driver_tests(void);
void run_buzzer_driver_tests(void);
void run_display_driver_tests(void);
void run_button_manager_tests(void);
void run_voice_recognition_tests(void);
void run_integration_tests(void);
void run_system_tests(void);

void setup() {
    delay(2000); // Wait for serial connection
    Serial.begin(9600);
    Serial.println("Starting wheelchair swing test suite...");
    Wire.begin();

    UNITY_BEGIN();
    Serial.println("===============================");
    Serial.println("Detecting available components:");
    Serial.println("===============================");

    // Always run state machine tests (software component)
    Serial.println("Running State Machine tests (software component)");
    run_state_machine_tests();

    // Run hardware-dependent tests based on component availability
    if (ComponentDetector::isUltrasonicAvailable(PIN_ULTRASONIC1_TRIG, PIN_ULTRASONIC1_ECHO)) {
        Serial.println("Front ultrasonic sensor detected - running tests");
        run_ultrasonic_tests();
    } else {
        Serial.println("Front ultrasonic sensor NOT detected - skipping tests");
    }

    if (ComponentDetector::isPressureSensorAvailable(PIN_PRESSURE_SENSOR)) {
        Serial.println("Pressure sensor detected - running tests");
        run_pressure_sensor_tests();
    } else {
        Serial.println("Pressure sensor NOT detected - skipping tests");
    }

    if (ComponentDetector::isMotorDriverAvailable(PIN_STEPPER1_IN1, PIN_STEPPER1_IN2,
                                                 PIN_STEPPER1_IN3, PIN_STEPPER1_IN4)) {
        Serial.println("Stepper motor driver detected - running tests");
        run_stepper_driver_tests();
    } else {
        Serial.println("Stepper motor driver NOT detected - skipping tests");
    }

    if (ComponentDetector::isActuatorAvailable(PIN_ACTUATOR_FWD, PIN_ACTUATOR_REV)) {
        Serial.println("Door actuator detected - running tests");
        run_actuator_driver_tests();
    } else {
        Serial.println("Door actuator NOT detected - skipping tests");
    }

    if (ComponentDetector::isBuzzerAvailable(PIN_BUZZER)) {
        Serial.println("Buzzer detected - running tests");
        run_buzzer_driver_tests();
    } else {
        Serial.println("Buzzer NOT detected - skipping tests");
    }

    if (ComponentDetector::isDisplayAvailable()) {
        Serial.println("Display detected - running tests");
        run_display_driver_tests();
    } else {
        Serial.println("Display NOT detected - skipping tests");
    }

    if (ComponentDetector::isVoiceModuleAvailable(PIN_VOICE_RX, PIN_VOICE_TX)) {
        Serial.println("Voice recognition module detected - running tests");
        run_voice_recognition_tests();
    } else {
        Serial.println("Voice recognition module NOT detected - skipping tests");
    }

    // SafetyMonitor tests use mock components when real ones aren't available
    Serial.println("Running Safety Monitor tests with available components/mocks");
    run_safety_monitor_tests();

    // Run integration tests if multiple components are available
    Serial.println("Running available integration tests");
    run_integration_tests();

    // Run system tests if all critical components are available
    if (ComponentDetector::isUltrasonicAvailable(PIN_ULTRASONIC1_TRIG, PIN_ULTRASONIC1_ECHO) &&
        ComponentDetector::isPressureSensorAvailable(PIN_PRESSURE_SENSOR) &&
        ComponentDetector::isMotorDriverAvailable(PIN_STEPPER1_IN1, PIN_STEPPER1_IN2,
                                               PIN_STEPPER1_IN3, PIN_STEPPER1_IN4)) {
        Serial.println("All critical components available - running system tests");
        run_system_tests();
    } else {
        Serial.println("Not all critical components available - skipping system tests");
    }

    UNITY_END();
}

void loop() {
    // Nothing to do after tests complete
}

// Example implementation of one test group
void run_state_machine_tests() {
    // Individual test case declarations would go here
    // RUN_TEST(test_state_machine_initial_state);
    // RUN_TEST(test_state_machine_start_transition);
    // RUN_TEST(test_state_machine_emergency);
    Serial.println("State machine tests completed");
}
