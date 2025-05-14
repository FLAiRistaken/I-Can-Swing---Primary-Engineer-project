// test/unit_tests/test_buzzer_driver.cpp
#include <unity.h>
#include "BuzzerDriver.h"
#include "Configuration.h"

// Mock Arduino functions
namespace {
    struct ToneCall {
        uint8_t pin;
        uint16_t frequency;
        uint32_t duration;
    };

    ToneCall lastToneCall = {0, 0, 0};
    ToneCall toneCallHistory[10] = {};
    int toneCallCount = 0;
    int noToneCallCount = 0;

    void resetMocks() {
        lastToneCall = {0, 0, 0};
        for (int i = 0; i < 10; i++) {
            toneCallHistory[i] = {0, 0, 0};
        }
        toneCallCount = 0;
        noToneCallCount = 0;
    }
}

// Mock tone function to track calls
void tone(uint8_t pin, uint16_t frequency, uint32_t duration) {
    lastToneCall = {pin, frequency, duration};
    if (toneCallCount < 10) {
        toneCallHistory[toneCallCount] = {pin, frequency, duration};
    }
    toneCallCount++;
}

// Mock noTone function
void noTone(uint8_t pin) {
    noToneCallCount++;
}

// Mock delay to avoid actual delays in tests
void delay(unsigned long ms) {
    // No actual delay for testing
}

// Test fixture
const uint8_t TEST_PIN = PIN_BUZZER;
BuzzerDriver buzzer(TEST_PIN);

void setUp(void) {
    resetMocks();
    buzzer.begin();
    resetMocks(); // Clear initialization calls
}

void tearDown(void) {
    // Cleanup
}

// Test cases
void test_buzzer_initialization() {
    buzzer.begin();
    TEST_ASSERT_EQUAL(1, noToneCallCount);
}

void test_single_beep() {
    buzzer.beep();

    TEST_ASSERT_EQUAL(TEST_PIN, lastToneCall.pin);
    TEST_ASSERT_EQUAL(1000, lastToneCall.frequency); // Default frequency
    TEST_ASSERT_EQUAL(100, lastToneCall.duration);   // Default duration
    TEST_ASSERT_EQUAL(1, toneCallCount);
}

void test_custom_beep() {
    const uint16_t testFreq = 2000;
    const uint32_t testDuration = 500;

    buzzer.beep(testFreq, testDuration);

    TEST_ASSERT_EQUAL(TEST_PIN, lastToneCall.pin);
    TEST_ASSERT_EQUAL(testFreq, lastToneCall.frequency);
    TEST_ASSERT_EQUAL(testDuration, lastToneCall.duration);
}

void test_play_tone() {
    const uint16_t testFreq = 440;
    const uint32_t testDuration = 200;

    buzzer.playTone(testFreq, testDuration);

    TEST_ASSERT_EQUAL(TEST_PIN, lastToneCall.pin);
    TEST_ASSERT_EQUAL(testFreq, lastToneCall.frequency);
    TEST_ASSERT_EQUAL(testDuration, lastToneCall.duration);
}

void test_stop_sound() {
    buzzer.stop();
    TEST_ASSERT_EQUAL(1, noToneCallCount);
}

void test_play_melody() {
    // Define a simple melody (first few notes of "Happy Birthday")
    const unsigned int frequencies[] = {262, 262, 294, 262, 349, 330};
    const unsigned long durations[] = {200, 200, 400, 400, 400, 800};
    const int noteCount = 6;

    buzzer.playMelody(frequencies, durations, noteCount);

    // Verify total call count
    TEST_ASSERT_EQUAL(noteCount, toneCallCount);

    // Verify correct frequencies and durations for each note
    for (int i = 0; i < noteCount && i < 10; i++) {
        TEST_ASSERT_EQUAL(TEST_PIN, toneCallHistory[i].pin);
        TEST_ASSERT_EQUAL(frequencies[i], toneCallHistory[i].frequency);
        TEST_ASSERT_EQUAL(durations[i], toneCallHistory[i].duration);
    }

    // Last tone call should match final note
    TEST_ASSERT_EQUAL(frequencies[noteCount-1], lastToneCall.frequency);
    TEST_ASSERT_EQUAL(durations[noteCount-1], lastToneCall.duration);
}

void test_alert_pattern() {
    // Test alert pattern: alternating high/low beeps
    buzzer.beep(2000, 200);  // High
    buzzer.beep(1000, 200);  // Low
    buzzer.beep(2000, 200);  // High

    TEST_ASSERT_EQUAL(3, toneCallCount);
    TEST_ASSERT_EQUAL(2000, toneCallHistory[0].frequency);
    TEST_ASSERT_EQUAL(1000, toneCallHistory[1].frequency);
    TEST_ASSERT_EQUAL(2000, toneCallHistory[2].frequency);
}

void test_ascending_pattern() {
    // Test ascending tone pattern
    buzzer.beep(500, 100);
    buzzer.beep(1000, 100);
    buzzer.beep(1500, 100);
    buzzer.beep(2000, 100);

    TEST_ASSERT_EQUAL(4, toneCallCount);
    TEST_ASSERT_EQUAL(500, toneCallHistory[0].frequency);
    TEST_ASSERT_EQUAL(1000, toneCallHistory[1].frequency);
    TEST_ASSERT_EQUAL(1500, toneCallHistory[2].frequency);
    TEST_ASSERT_EQUAL(2000, toneCallHistory[3].frequency);
}

void run_buzzer_driver_tests() {
    UNITY_BEGIN();
    RUN_TEST(test_buzzer_initialization);
    RUN_TEST(test_single_beep);
    RUN_TEST(test_custom_beep);
    RUN_TEST(test_play_tone);
    RUN_TEST(test_stop_sound);
    RUN_TEST(test_play_melody);
    RUN_TEST(test_alert_pattern);
    RUN_TEST(test_ascending_pattern);
    UNITY_END();
}

void setup() {
    delay(2000);
    run_buzzer_driver_tests();
}

void loop() {
    // Nothing to do here
}
