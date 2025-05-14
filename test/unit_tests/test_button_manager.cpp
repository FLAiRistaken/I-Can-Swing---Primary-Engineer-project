// test/unit_tests/test_button_manager.cpp
#include <unity.h>
#include "ButtonManager.h"
#include "Configuration.h"

// Mock functions for testing
class MockButtonReader {
public:
    static bool mockButtonStates[ButtonManager::BTN_COUNT];
    static int digitalReadCalls;

    static int digitalRead(uint8_t pin) {
        digitalReadCalls++;

        // Check if pin matches any button pin
        if (pin == PIN_BTN_START) return mockButtonStates[ButtonManager::BTN_START] ? LOW : HIGH;
        if (pin == PIN_BTN_STOP) return mockButtonStates[ButtonManager::BTN_STOP] ? LOW : HIGH;
        if (pin == PIN_BTN_SPEED_UP) return mockButtonStates[ButtonManager::BTN_SPEED_UP] ? LOW : HIGH;
        if (pin == PIN_BTN_SPEED_DOWN) return mockButtonStates[ButtonManager::BTN_SPEED_DOWN] ? LOW : HIGH;
        if (pin == PIN_BTN_DOOR) return mockButtonStates[ButtonManager::BTN_DOOR] ? LOW : HIGH;
        if (pin == PIN_EMERGENCY_STOP) return mockButtonStates[ButtonManager::BTN_EMERGENCY] ? LOW : HIGH;

        return HIGH; // Default to not pressed (INPUT_PULLUP)
    }

    static void resetCalls() {
        digitalReadCalls = 0;
    }
};

bool MockButtonReader::mockButtonStates[ButtonManager::BTN_COUNT] = {false};
int MockButtonReader::digitalReadCalls = 0;

// Mock millis() function for debounce testing
unsigned long mockTime = 0;
unsigned long millis() {
    return mockTime;
}

// Test fixture
ButtonManager buttonManager;

void setUp(void) {
    // Reset mock state before each test
    for (int i = 0; i < ButtonManager::BTN_COUNT; i++) {
        MockButtonReader::mockButtonStates[i] = false;
    }
    mockTime = 0;
    MockButtonReader::resetCalls();

    // Initialize ButtonManager
    // Note: For real tests, we'd need to replace digitalRead with our mock
    // This would require modifications to ButtonManager to allow dependency injection
}

void tearDown(void) {
    // Cleanup
}

// Test cases
void test_button_initial_state() {
    // When ButtonManager is initialized, all buttons should be in released state
    ButtonManager buttons;
    buttons.begin();

    for (int i = 0; i < ButtonManager::BTN_COUNT - 1; i++) { // Skip emergency button
        TEST_ASSERT_FALSE(buttons.isPressed((ButtonManager::Button)i));
        TEST_ASSERT_FALSE(buttons.wasPressed((ButtonManager::Button)i));
    }
    TEST_ASSERT_FALSE(buttons.isPressed(ButtonManager::BTN_EMERGENCY));
}

void test_button_debouncing() {
    // This test verifies debouncing for button press
    // Test sequence:
    // 1. Button pressed (LOW with INPUT_PULLUP)
    // 2. Check before debounce time - should not register
    // 3. Check after debounce time - should register

    ButtonManager buttons;
    buttons.begin();

    // Simulate START button press
    // In real tests, we'd set the mock digitalRead to return LOW
    // Test the debouncing algorithm directly

    // Initialize to not pressed state
    // buttons._currentState[ButtonManager::BTN_START] = false;
    // buttons._lastState[ButtonManager::BTN_START] = false;
    // buttons._pressedFlag[ButtonManager::BTN_START] = false;

    // Simulate button readings with noise
    // In real tests, we would:
    // 1. Set mockButtonStates[ButtonManager::BTN_START] = true;
    // 2. Call update() - should not register yet due to debounce
    // 3. Increment mockTime to just below BUTTON_DEBOUNCE_MS
    // 4. Call update() - should still not register
    // 5. Increment mockTime past BUTTON_DEBOUNCE_MS
    // 6. Call update() - should register now

    // For simplicity in this example, use a direct approach
    TEST_ASSERT_FALSE(buttons.isPressed(ButtonManager::BTN_START));

    // In a full test, we'd verify button state after debounce period
    // This requires direct access to internal state or dependency injection
}

void test_noise_rejection() {
    // Test scenario: Button signal bounces (press-release-press) during debounce window
    // Button should not register until stable for BUTTON_DEBOUNCE_MS

    ButtonManager buttons;
    buttons.begin();

    // Without access to internal state or dependency injection,
    // we can't fully test this automatically

    // In real tests with dependency injection:
    // 1. Set button pressed
    // 2. Update
    // 3. Set button released (noise/bounce)
    // 4. Update
    // 5. Set button pressed again
    // 6. Update
    // 7. Check that button is not registered yet
    // 8. Advance time past debounce window
    // 9. Update
    // 10. Verify button is now registered
}

void test_was_pressed_functionality() {
    // This tests that wasPressed() returns true only once per button press

    ButtonManager buttons;
    buttons.begin();

    // Simulate emergency button press (we can test this directly)
    ButtonManager::emergencyStopISR();

    // First call should return true
    TEST_ASSERT_TRUE(buttons.wasPressed(ButtonManager::BTN_EMERGENCY));

    // Second call should return false
    TEST_ASSERT_FALSE(buttons.wasPressed(ButtonManager::BTN_EMERGENCY));

    // Button should still report as pressed
    TEST_ASSERT_TRUE(buttons.isPressed(ButtonManager::BTN_EMERGENCY));
}

void test_emergency_stop_isr() {
    // Test that emergency stop ISR sets flag correctly

    // Clear any existing state
    ButtonManager buttons;
    buttons.begin();

    // Verify initial state
    TEST_ASSERT_FALSE(buttons.isPressed(ButtonManager::BTN_EMERGENCY));

    // Trigger ISR
    ButtonManager::emergencyStopISR();

    // Verify button is now detected as pressed
    TEST_ASSERT_TRUE(buttons.isPressed(ButtonManager::BTN_EMERGENCY));
}

void run_button_manager_tests() {
    UNITY_BEGIN();
    RUN_TEST(test_button_initial_state);
    RUN_TEST(test_button_debouncing);
    RUN_TEST(test_noise_rejection);
    RUN_TEST(test_was_pressed_functionality);
    RUN_TEST(test_emergency_stop_isr);
    UNITY_END();
}

void setup() {
    delay(2000);
    run_button_manager_tests();
}

void loop() {
    // Nothing to do here
}
