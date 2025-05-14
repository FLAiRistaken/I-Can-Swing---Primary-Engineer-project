// test/unit_tests/test_display_driver.cpp
#include <unity.h>
#include "DisplayDriver.h"

// Mock U8G2 library to intercept calls
namespace {
    struct U8G2CallHistory {
        bool beginCalled = false;
        bool clearBufferCalled = false;
        bool sendBufferCalled = false;
        bool drawStrCalled = false;
        bool setFontCalled = false;
        bool getStrWidthCalled = false;

        uint8_t lastX = 0;
        uint8_t lastY = 0;
        const char* lastText = nullptr;
        const uint8_t* lastFont = nullptr;
        uint16_t strWidthToReturn = 0;
    };

    U8G2CallHistory mockCalls;
}

// Mock implementation of U8G2
// We use preprocessor definitions to redirect U8G2 method calls to our mock functions
#define U8G2_SH1106_128X64_NONAME_F_HW_I2C MockU8G2
#define u8g2_font_6x10_tf nullptr

class MockU8G2 {
public:
    MockU8G2(uint8_t rotation, uint8_t resetPin) {}

    void begin() {
        mockCalls.beginCalled = true;
    }

    void clearBuffer() {
        mockCalls.clearBufferCalled = true;
    }

    void sendBuffer() {
        mockCalls.sendBufferCalled = true;
    }

    void drawStr(uint8_t x, uint8_t y, const char* text) {
        mockCalls.drawStrCalled = true;
        mockCalls.lastX = x;
        mockCalls.lastY = y;
        mockCalls.lastText = text;
    }

    void setFont(const uint8_t* font) {
        mockCalls.setFontCalled = true;
        mockCalls.lastFont = font;
    }

    uint16_t getStrWidth(const char* text) {
        mockCalls.getStrWidthCalled = true;
        mockCalls.lastText = text;
        return mockCalls.strWidthToReturn;
    }

    void setFontRefHeightExtendedText() {}
    void setFontPosTop() {}
    void setFontMode(uint8_t mode) {}
    void sendF(const char* fmt, ...) {}
};

void setUp(void) {
    // Reset mock state before each test
    mockCalls = {};
}

void tearDown(void) {
    // Clean up after test
}

void test_initialization(void) {
    DisplayDriver display;
    display.begin();

    TEST_ASSERT_TRUE(mockCalls.beginCalled);
    TEST_ASSERT_TRUE(mockCalls.clearBufferCalled);
    TEST_ASSERT_TRUE(mockCalls.sendBufferCalled);
}

void test_clear(void) {
    DisplayDriver display;
    display.begin();

    // Reset tracking
    mockCalls = {};

    display.clear();
    TEST_ASSERT_TRUE(mockCalls.clearBufferCalled);
    TEST_ASSERT_FALSE(mockCalls.sendBufferCalled); // Should not auto-send
}

void test_display(void) {
    DisplayDriver display;
    display.begin();

    // Reset tracking
    mockCalls = {};

    display.display();
    TEST_ASSERT_TRUE(mockCalls.sendBufferCalled);
}

void test_draw_text(void) {
    DisplayDriver display;
    display.begin();

    // Reset tracking
    mockCalls = {};

    const char* testText = "Hello World";
    uint8_t testX = 10;
    uint8_t testY = 20;

    display.drawText(testX, testY, testText);

    TEST_ASSERT_TRUE(mockCalls.drawStrCalled);
    TEST_ASSERT_EQUAL(testX, mockCalls.lastX);
    TEST_ASSERT_EQUAL(testY, mockCalls.lastY);
    TEST_ASSERT_EQUAL_STRING(testText, mockCalls.lastText);
}

void test_set_font(void) {
    DisplayDriver display;
    display.begin();

    // Reset tracking
    mockCalls = {};

    const uint8_t* testFont = (const uint8_t*)0x12345678; // Arbitrary pointer

    display.setFont(testFont);

    TEST_ASSERT_TRUE(mockCalls.setFontCalled);
    TEST_ASSERT_EQUAL(testFont, mockCalls.lastFont);
}

void test_show_text_centered(void) {
    DisplayDriver display;
    display.begin();

    // Reset tracking
    mockCalls = {};

    const char* testText = "Centered Text";
    mockCalls.strWidthToReturn = 60; // Simulate text width of 60 pixels

    display.showText(testText);

    TEST_ASSERT_TRUE(mockCalls.clearBufferCalled);
    TEST_ASSERT_TRUE(mockCalls.getStrWidthCalled);
    TEST_ASSERT_TRUE(mockCalls.drawStrCalled);
    TEST_ASSERT_TRUE(mockCalls.sendBufferCalled);

    // Text should be centered: (128 - 60) / 2 = 34
    TEST_ASSERT_EQUAL(34, mockCalls.lastX);
    TEST_ASSERT_EQUAL(32, mockCalls.lastY);
    TEST_ASSERT_EQUAL_STRING(testText, mockCalls.lastText);
}

void run_display_driver_tests() {
    UNITY_BEGIN();
    RUN_TEST(test_initialization);
    RUN_TEST(test_clear);
    RUN_TEST(test_display);
    RUN_TEST(test_draw_text);
    RUN_TEST(test_set_font);
    RUN_TEST(test_show_text_centered);
    UNITY_END();
}

void setup() {
    delay(2000);
    run_display_driver_tests();
}

void loop() {
    // Nothing to do here
}
