// test_actuator_l298n.cpp - Enhanced with initialization and full range testing
#include <Arduino.h>
#include <Wire.h>
#include "Configuration.h"
#include "BuzzerDriver.h"
#include "ExpanderManager.h"
#include "ActuatorDriver.h"

// Create component instances
BuzzerDriver buzzer(PIN_BUZZER);
ExpanderManager expander;

// Create TWO actuator driver instances
ActuatorDriver actuator1(5, 6, nullptr, &expander);
ActuatorDriver actuator2(7, 8, nullptr, &expander);

void setup() {
    Serial.begin(9600);
    Serial.println("Enhanced Dual Actuator Test with L298N via Expander");
    Serial.println("===================================================");
    Serial.println("Actuator 1: Expander pins 5,6");
    Serial.println("Actuator 2: Expander pins 7,8");
    Serial.println("");

    // Initialize I2C for the expander
    Wire.setClock(100000);
    Wire.begin();

    // Initialize buzzer
    buzzer.begin();

    // Initialize expander
    Serial.println("Initializing MCP23017 expander...");
    if (!expander.begin()) {
        Serial.println("FATAL: Expander chip not found. Check connections!");
        buzzer.beep(300, 1000);
        while(1);
    }
    Serial.println("Expander initialized successfully.");

    // Initialize both actuator drivers
    Serial.println("Initializing actuator 1...");
    actuator1.begin();
    Serial.println("Actuator 1 initialized.");

    Serial.println("Initializing actuator 2...");
    actuator2.begin();
    Serial.println("Actuator 2 initialized.");

    // Confirmation beeps
    buzzer.beep(1000, 100);
    delay(100);
    buzzer.beep(1200, 100);
    delay(100);
    buzzer.beep(1500, 100);

    // === INITIALIZATION SEQUENCE ===
    Serial.println("\n=== INITIALIZATION: Ensuring Full Retraction ===");
    Serial.println("Retracting both actuators to ensure known starting position...");

    // Start both actuators retracting for extended time to ensure full retraction
    actuator1.startRetract(16000);  // 10 seconds to ensure full retraction
    actuator2.startRetract(16000);  // 10 seconds to ensure full retraction

    buzzer.beep(500, 200);  // Low tone for initialization

    // Monitor initialization retraction
    bool act1Retracting = true, act2Retracting = true;
    unsigned long initStartTime = millis();

    while (act1Retracting || act2Retracting) {
        actuator1.update();
        actuator2.update();

        bool act1Moving = actuator1.isMoving();
        bool act2Moving = actuator2.isMoving();

        if (act1Retracting && !act1Moving) {
            Serial.println("Actuator 1 fully retracted!");
            buzzer.beep(800, 100);
            act1Retracting = false;
        }
        if (act2Retracting && !act2Moving) {
            Serial.println("Actuator 2 fully retracted!");
            buzzer.beep(900, 100);
            act2Retracting = false;
        }

        // Status update every 2 seconds
        if ((millis() - initStartTime) % 2000 < 50) {
            Serial.print("Initialization progress: ");
            Serial.print((millis() - initStartTime) / 1000.0, 1);
            Serial.println("s");
        }

        delay(50);
    }

    Serial.println("✓ Both actuators fully retracted and ready!");
    buzzer.beep(1200, 100);
    delay(100);
    buzzer.beep(1400, 100);

    Serial.println("\nTest sequence starting in 3 seconds...");
    delay(3000);
}

void loop() {
    Serial.println("\n=== Starting Enhanced Actuator Test Cycle ===");

    // Test 1: Individual actuator testing
    Serial.println("\n--- Test 1: Individual Actuator Operations ---");

    Serial.println("Actuator 1: Extending for 2 seconds");
    actuator1.extend();
    buzzer.beep(800, 100);
    delay(2000);
    actuator1.stop();
    buzzer.beep(600, 50);
    delay(500);

    Serial.println("Actuator 2: Extending for 2 seconds");
    actuator2.extend();
    buzzer.beep(900, 100);
    delay(2000);
    actuator2.stop();
    buzzer.beep(600, 50);
    delay(500);

    Serial.println("Actuator 1: Retracting for 2 seconds");
    actuator1.retract();
    buzzer.beep(800, 100);
    delay(2000);
    actuator1.stop();
    buzzer.beep(600, 50);
    delay(500);

    Serial.println("Actuator 2: Retracting for 2 seconds");
    actuator2.retract();
    buzzer.beep(900, 100);
    delay(2000);
    actuator2.stop();
    buzzer.beep(600, 50);
    delay(1000);

    // Test 2: FULL EXTENSION TEST
    Serial.println("\n--- Test 2: FULL EXTENSION TEST ---");
    Serial.println("Testing maximum extension range for both actuators");
    Serial.println("WARNING: Ensure actuators have clear path for full extension!");

    delay(2000);  // Give time to read warning

    Serial.println("Both actuators: FULL EXTENSION (10 seconds)");
    actuator1.startExtend(16000);  // 10 seconds for full extension
    actuator2.startExtend(16000);  // 10 seconds for full extension

    buzzer.beep(1500, 200);  // High tone for full extension test
    delay(100);
    buzzer.beep(1500, 200);

    // Monitor full extension
    bool act1Extending = true, act2Extending = true;
    unsigned long extStartTime = millis();

    while (act1Extending || act2Extending) {
        actuator1.update();
        actuator2.update();

        bool act1Moving = actuator1.isMoving();
        bool act2Moving = actuator2.isMoving();

        if (act1Extending && !act1Moving) {
            Serial.println("✓ Actuator 1 FULLY EXTENDED!");
            buzzer.beep(1000, 150);
            act1Extending = false;
        }
        if (act2Extending && !act2Moving) {
            Serial.println("✓ Actuator 2 FULLY EXTENDED!");
            buzzer.beep(1100, 150);
            act2Extending = false;
        }

        // Progress update every 1 second during full extension
        if ((millis() - extStartTime) % 1000 < 50) {
            Serial.print("Full extension progress: ");
            Serial.print((millis() - extStartTime) / 1000.0, 1);
            Serial.print("s / 10.0s");
            if (act1Moving) Serial.print(" [Act1: extending]");
            if (act2Moving) Serial.print(" [Act2: extending]");
            Serial.println();
        }

        delay(50);
    }

    Serial.println("✓ FULL EXTENSION TEST COMPLETE!");
    buzzer.beep(1200, 100);
    delay(100);
    buzzer.beep(1400, 100);
    delay(100);
    buzzer.beep(1600, 100);

    delay(2000);  // Hold at full extension for observation

    // Return to retracted position after full extension test
    Serial.println("Returning both actuators to retracted position...");
    actuator1.startRetract(16000);  // Full retraction time
    actuator2.startRetract(16000);  // Full retraction time

    while (actuator1.isMoving() || actuator2.isMoving()) {
        actuator1.update();
        actuator2.update();
        delay(50);
    }

    Serial.println("✓ Both actuators returned to retracted position");
    buzzer.beep(800, 100);
    delay(1000);

    // Test 3: Simultaneous operation - same direction
    Serial.println("\n--- Test 3: Simultaneous Extension ---");
    Serial.println("Both actuators extending together");
    actuator1.extend();
    actuator2.extend();
    buzzer.beep(1000, 150);
    delay(2500);
    actuator1.stop();
    actuator2.stop();
    buzzer.beep(600, 100);
    delay(1000);

    Serial.println("Both actuators retracting together");
    actuator1.retract();
    actuator2.retract();
    buzzer.beep(1000, 150);
    delay(2500);
    actuator1.stop();
    actuator2.stop();
    buzzer.beep(600, 100);
    delay(1000);

    // Test 4: Opposite direction operation
    Serial.println("\n--- Test 4: Opposite Direction Operation ---");
    Serial.println("Actuator 1 extending, Actuator 2 retracting");
    actuator1.extend();
    actuator2.retract();
    buzzer.beep(1100, 100);
    delay(100);
    buzzer.beep(900, 100);
    delay(2000);
    actuator1.stop();
    actuator2.stop();
    delay(500);

    Serial.println("Actuator 1 retracting, Actuator 2 extending");
    actuator1.retract();
    actuator2.extend();
    buzzer.beep(900, 100);
    delay(100);
    buzzer.beep(1100, 100);
    delay(2000);
    actuator1.stop();
    actuator2.stop();
    delay(1000);

    // Test 5: Timed operations
    Serial.println("\n--- Test 5: Timed Operations ---");

    Serial.println("Actuator 1: Timed extend (3 seconds)");
    actuator1.startExtend(3000);
    buzzer.beep(1200, 100);

    unsigned long startTime = millis();
    while (actuator1.isMoving()) {
        actuator1.update();
        if ((millis() - startTime) % 500 < 50) {
            Serial.print("Actuator 1 extending... ");
            Serial.print((millis() - startTime) / 1000.0, 1);
            Serial.println("s");
        }
        delay(50);
    }
    Serial.println("Actuator 1 timed extend complete!");
    buzzer.beep(1400, 100);
    delay(500);

    Serial.println("Actuator 2: Timed extend (3 seconds)");
    actuator2.startExtend(3000);
    buzzer.beep(1200, 100);

    startTime = millis();
    while (actuator2.isMoving()) {
        actuator2.update();
        if ((millis() - startTime) % 500 < 50) {
            Serial.print("Actuator 2 extending... ");
            Serial.print((millis() - startTime) / 1000.0, 1);
            Serial.println("s");
        }
        delay(50);
    }
    Serial.println("Actuator 2 timed extend complete!");
    buzzer.beep(1400, 100);
    delay(1000);

    // Test 6: Status reporting
    Serial.println("\n--- Test 6: Status Reporting ---");

    Serial.println("Actuator 1 Status:");
    Serial.print("  Direction: ");
    ActuatorDriver::Direction dir1 = actuator1.getCurrentDirection();
    switch (dir1) {
        case ActuatorDriver::DIRECTION_STOP: Serial.println("STOPPED"); break;
        case ActuatorDriver::DIRECTION_EXTEND: Serial.println("EXTENDING"); break;
        case ActuatorDriver::DIRECTION_RETRACT: Serial.println("RETRACTING"); break;
    }
    Serial.print("  Is moving: "); Serial.println(actuator1.isMoving() ? "YES" : "NO");

    Serial.println("Actuator 2 Status:");
    Serial.print("  Direction: ");
    ActuatorDriver::Direction dir2 = actuator2.getCurrentDirection();
    switch (dir2) {
        case ActuatorDriver::DIRECTION_STOP: Serial.println("STOPPED"); break;
        case ActuatorDriver::DIRECTION_EXTEND: Serial.println("EXTENDING"); break;
        case ActuatorDriver::DIRECTION_RETRACT: Serial.println("RETRACTING"); break;
    }
    Serial.print("  Is moving: "); Serial.println(actuator2.isMoving() ? "YES" : "NO");

    // Final completion sequence
    buzzer.beep(1000, 100);
    delay(100);
    buzzer.beep(1200, 100);
    delay(100);
    buzzer.beep(1400, 100);
    delay(100);
    buzzer.beep(1600, 100);

    Serial.println("\n=== Enhanced Test Cycle Complete ===");
    Serial.println("Waiting 10 seconds before next cycle...\n");
    delay(10000);
}
