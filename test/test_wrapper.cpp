// test/test_wrapper.cpp
#include <Arduino.h>

// Custom buffer for test results
char testResultBuffer[1024] = {0};
int testResultIndex = 0;

// Function to print test results
void printTestResults() {
    Serial.println(F("===== TEST RESULTS ====="));
    Serial.println(testResultBuffer);
    Serial.println(F("======================="));
}

// Add this to setup() in test_minimal.cpp
void setupTestOutput() {
    Serial.begin(9600);
    delay(2000);
    testResultBuffer[0] = '\0';
    testResultIndex = 0;
}

// Call at the end of each test
void flushTestOutput() {
    printTestResults();
}
