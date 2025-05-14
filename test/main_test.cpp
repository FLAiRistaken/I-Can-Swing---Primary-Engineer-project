// test/main_test.cpp
#include <Arduino.h>
#include <unity.h>
#include "test_utils/component_detector.h"
#include "Configuration.h"

// Forward declarations of test runners
extern void run_state_machine_tests(void);
extern void run_safety_monitor_tests(void);
extern void run_ultrasonic_tests(void);
extern void run_pressure_sensor_tests(void);
extern void run_stepper_driver_tests(void);
extern void run_actuator_driver_tests(void);
extern void run_buzzer_driver_tests(void);
extern void run_display_driver_tests(void);
extern void run_voice_recognition_tests(void);

// Integration test runners
extern void run_safety_state_integration_tests(void);
extern void run_voice_control_integration_tests(void);
extern void run_user_presence_integration_tests(void);

// System test runners
extern void run_door_operation_tests(void);

// Test stats tracking
struct TestStats {
    int total = 0;
    int run = 0;
    int skipped = 0;
};

TestStats stats;

void runTestGroup(const char* name, void (*testFunction)(), bool condition) {
    stats.total++;

    if (condition) {
        Serial.print(F("\n> Running: "));
        Serial.println(name);
        stats.run++;
        testFunction();
    } else {
        Serial.print(F("\n> SKIPPED: "));
        Serial.print(name);
        Serial.println(F(" (hardware not detected)"));
        stats.skipped++;
    }
}

void setup() {
    delay(2000); // Wait for Serial to initialize
    Serial.begin(9600);
    Wire.begin(); // Initialize I2C for component detection

    Serial.println(F("\n\n===== Wheelchair Swing Test Suite ====="));
    Serial.println(F("Date: May 14, 2025"));

    // Detect available hardware components
    Serial.println(F("\nDetecting hardware components..."));
    ComponentDetector::printComponentStatus();

    // Check for core hardware components
    bool frontUltrasonic = ComponentDetector::isUltrasonicAvailable(PIN_ULTRASONIC1_TRIG, PIN_ULTRASONIC1_ECHO);
    bool rearUltrasonic = ComponentDetector::isUltrasonicAvailable(PIN_ULTRASONIC2_TRIG, PIN_ULTRASONIC2_ECHO);
    bool pressureSensor = ComponentDetector::isPressureSensorAvailable(PIN_PRESSURE_SENSOR);
    bool stepperMotor = ComponentDetector::isStepperDriverAvailable(PIN_STEPPER1_IN1, PIN_STEPPER1_IN2,
                                                                  PIN_STEPPER1_IN3, PIN_STEPPER1_IN4);
    bool actuator = ComponentDetector::isActuatorAvailable(PIN_ACTUATOR_FWD, PIN_ACTUATOR_REV);
    bool display = ComponentDetector::isDisplayAvailable();
    bool voiceModule = ComponentDetector::isVoiceModuleAvailable(PIN_VOICE_RX, PIN_VOICE_TX);
    bool buzzer = ComponentDetector::isBuzzerAvailable(PIN_BUZZER);

    // Run hardware-independent tests (always run)
    Serial.println(F("\n--- Running Hardware-Independent Tests ---"));
    runTestGroup("State Machine Tests", run_state_machine_tests, true);
    runTestGroup("Safety Monitor Tests", run_safety_monitor_tests, true);

    // Run hardware-dependent unit tests if components are available
    Serial.println(F("\n--- Running Hardware-Dependent Unit Tests ---"));
    runTestGroup("Ultrasonic Sensor Tests", run_ultrasonic_tests, frontUltrasonic || rearUltrasonic);
    runTestGroup("Pressure Sensor Tests", run_pressure_sensor_tests, pressureSensor);
    runTestGroup("Stepper Driver Tests", run_stepper_driver_tests, stepperMotor);
    runTestGroup("Actuator Driver Tests", run_actuator_driver_tests, actuator);
    runTestGroup("Buzzer Driver Tests", run_buzzer_driver_tests, buzzer);
    runTestGroup("Display Driver Tests", run_display_driver_tests, display);
    runTestGroup("Voice Recognition Tests", run_voice_recognition_tests, voiceModule);

    // Run integration tests if multiple components are available
    Serial.println(F("\n--- Running Integration Tests ---"));
    runTestGroup("Safety & State Integration Tests", run_safety_state_integration_tests,
                 frontUltrasonic && pressureSensor);
    runTestGroup("Voice Control Integration Tests", run_voice_control_integration_tests,
                 voiceModule && stepperMotor);
    runTestGroup("User Presence Integration Tests", run_user_presence_integration_tests,
                 pressureSensor);

    // Run system tests if all critical components are available
    Serial.println(F("\n--- Running System Tests ---"));
    runTestGroup("Door Operation System Tests", run_door_operation_tests,
                 actuator && (frontUltrasonic || rearUltrasonic));

    // Print test summary
    Serial.println(F("\n===== Test Summary ====="));
    Serial.print(F("Total test groups: "));
    Serial.println(stats.total);
    Serial.print(F("Run: "));
    Serial.println(stats.run);
    Serial.print(F("Skipped: "));
    Serial.println(stats.skipped);

    // Visual indicator of completion
    if (buzzer) {
        // Three beeps for success
        tone(PIN_BUZZER, 1000, 100);
        delay(150);
        tone(PIN_BUZZER, 1200, 100);
        delay(150);
        tone(PIN_BUZZER, 1500, 100);
    }
}

void loop() {
    // Nothing to do in loop
    delay(1000);
}
