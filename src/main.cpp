#include <Arduino.h>
#include <Wire.h>
#include "Configuration.h"
#include "BuzzerDriver.h"
#include "DisplayDriver.h"
#include "ButtonManager.h"
#include "StateMachine.h"
#include "StepperDriver.h"
#include "UltrasonicSensor.h"
#include "PressureSensor.h"
#include "ActuatorDriver.h"
#include "SafetyMonitor.h"


// Create SafetyMonitor instance
SafetyMonitor safetyMonitor(&stateMachine, &ultrasonicFront, &ultrasonicRear, &pressureSensor);

// Create component instances
BuzzerDriver buzzer(PIN_BUZZER);
DisplayDriver display;
ButtonManager buttons;
StateMachine stateMachine;
// Create stepper motor drivers
StepperDriver stepperLeft(PIN_STEPPER1_STEP, PIN_STEPPER1_DIR, PIN_STEPPER1_ENABLE);
StepperDriver stepperRight(PIN_STEPPER2_STEP, PIN_STEPPER2_DIR, PIN_STEPPER2_ENABLE);
// Create ultrasonic sensor instances
UltrasonicSensor ultrasonicFront(PIN_ULTRASONIC1_TRIG, PIN_ULTRASONIC1_ECHO, "Front");
UltrasonicSensor ultrasonicRear(PIN_ULTRASONIC2_TRIG, PIN_ULTRASONIC2_ECHO, "Rear");
PressureSensor pressureSensor(PIN_PRESSURE_SENSOR, PRESSURE_THRESHOLD, "BasketSensor");
ActuatorDriver doorActuator(PIN_ACTUATOR_FWD, PIN_ACTUATOR_REV, &stateMachine);


// UsS Distance values
float frontDistance = 0.0;
float rearDistance = 0.0;

// Timing variables
unsigned long lastDisplayUpdate = 0;
unsigned long lastSensorCheck = 0;


void updateDisplay() {
    display.clear();

    // Show state and speed on display
    char statusLine[32];
    sprintf(statusLine, "Status: %s", stateMachine.getStateString());
    display.drawText(0, 0, statusLine);

    char speedLine[32];
    sprintf(speedLine, "Speed: %s", stateMachine.getSpeedString());
    display.drawText(0, 16, speedLine);

    // Show ultrasonic sensor values
    char distanceLine[32];
    sprintf(distanceLine, "Dist F:%0.1f R:%0.1f cm",
        safetyMonitor.getFrontDistance(),
        safetyMonitor.getRearDistance());
    display.drawText(0, 32, distanceLine);

    display.display();
}

void handleButtons() {
    buttons.update();

    // Map button presses to state machine events
    if (buttons.wasPressed(ButtonManager::BTN_START)) {
        stateMachine.processEvent(StateMachine::EVENT_START_PRESSED);
    }

    if (buttons.wasPressed(ButtonManager::BTN_STOP)) {
        stateMachine.processEvent(StateMachine::EVENT_STOP_PRESSED);
    }

    if (buttons.wasPressed(ButtonManager::BTN_SPEED_UP)) {
        stateMachine.processEvent(StateMachine::EVENT_SPEED_UP);
    }

    if (buttons.wasPressed(ButtonManager::BTN_SPEED_DOWN)) {
        stateMachine.processEvent(StateMachine::EVENT_SPEED_DOWN);
    }

    if (buttons.wasPressed(ButtonManager::BTN_DOOR)) {
        stateMachine.processEvent(StateMachine::EVENT_DOOR_TOGGLE);
    }

    if (buttons.wasPressed(ButtonManager::BTN_EMERGENCY)) {
        stateMachine.processEvent(StateMachine::EVENT_EMERGENCY);
        buzzer.playTone(2000, 1000);  // Emergency alert
    }
    // Handle error state clearing
    if (stateMachine.getCurrentState() == StateMachine::STATE_ERROR) {
        if (buttons.wasPressed(ButtonManager::BTN_STOP)) {
            Serial.println("ERROR state cleared by STOP button");
            stateMachine.processEvent(StateMachine::EVENT_ERROR_CLEARED);
            buzzer.beep(1000, 100); // Confirmation beep
        }
    }
    // Handle emergency reset
    static unsigned long emergencyResetStartTime = 0;
    if (stateMachine.getCurrentState() == StateMachine::STATE_EMERGENCY) {
        // First check that physical button has been reset (not pressed)
        if (!digitalRead(PIN_EMERGENCY_STOP)) { // Assuming active LOW for emergency button
            // Button has been physically reset, now check for software reset
            if (buttons.isPressed(ButtonManager::BTN_STOP)) {
                if (emergencyResetStartTime == 0) {
                    // Start timing when button first pressed
                    emergencyResetStartTime = millis();
                    buzzer.beep(300, 100); // Feedback beep
                } else if (millis() - emergencyResetStartTime > 3000) {
                    // Button held for 3+ seconds, trigger reset
                    stateMachine.processEvent(StateMachine::EVENT_EMERGENCY_RESET);
                    buzzer.beep(700, 100); // Success indication
                    delay(100);
                    buzzer.beep(1200, 100);
                    emergencyResetStartTime = 0;
                }
            } else {
                emergencyResetStartTime = 0; // Reset timer if button released
            }
        }
    }
}

void checkSensors() {
    // Check pressure sensor for occupancy
    static bool lastUserPresentState = false;
    bool currentUserPresent = pressureSensor.isOccupied(); // Use the class method

    if (currentUserPresent != lastUserPresentState) {
        lastUserPresentState = currentUserPresent;
        stateMachine.processEvent(currentUserPresent ?
                                  StateMachine::EVENT_PRESSURE_ON :
                                  StateMachine::EVENT_PRESSURE_OFF);

        Serial.print("User presence changed: ");
        Serial.println(currentUserPresent ? "Present" : "Absent");
        buzzer.beep(800, 50); // Optional feedback beep
    }

}


// Function to check ultrasonic sensors
void checkUltrasonicSensors() {
    // Measure distances
    frontDistance = ultrasonicFront.measureDistance();
    rearDistance = ultrasonicRear.measureDistance();

    // Define thresholds
    const int CRITICAL_DISTANCE_CM = 10; // Very close - emergency
    const int WARNING_DISTANCE_CM = OBSTACLE_DISTANCE_CM; // Normal obstacle - error

    // Check for critical proximity (EMERGENCY condition)
    if ((frontDistance > 0 && frontDistance < CRITICAL_DISTANCE_CM) ||
        (rearDistance > 0 && rearDistance < CRITICAL_DISTANCE_CM)) {
        // Immediate danger detected - trigger emergency
        stateMachine.processEvent(StateMachine::EVENT_EMERGENCY);
        buzzer.playTone(2000, 500); // Urgent alert sound
        Serial.println("CRITICAL: Object extremely close! Emergency triggered.");
        return; // Exit after triggering emergency
    }

    // Check for obstacles (ERROR condition)
    if ((frontDistance > CRITICAL_DISTANCE_CM && frontDistance < WARNING_DISTANCE_CM) ||
        (rearDistance > CRITICAL_DISTANCE_CM && rearDistance < WARNING_DISTANCE_CM)) {
        // Obstacle detected - trigger error only if not already in error/emergency
        if (stateMachine.getCurrentState() != StateMachine::STATE_ERROR &&
            stateMachine.getCurrentState() != StateMachine::STATE_EMERGENCY) {
            stateMachine.processEvent(StateMachine::EVENT_OBSTACLE_DETECTED);
            buzzer.beep(1500, 100); // Alert sound
            Serial.print("WARNING: Object detected at ");
            Serial.print((frontDistance < WARNING_DISTANCE_CM) ? frontDistance : rearDistance);
            Serial.println(" cm");
        }
    }
}

void updateMotors() {
    // Set motor speeds based on current state and speed setting
    if (stateMachine.getCurrentState() == StateMachine::STATE_SWINGING) {
        uint16_t speedValue = 0;

        switch (stateMachine.getCurrentSpeed()) {
            case StateMachine::SPEED_LOW:
                speedValue = SPEED_LOW;
                break;
            case StateMachine::SPEED_MEDIUM:
                speedValue = SPEED_MEDIUM;
                break;
            case StateMachine::SPEED_HIGH:
                speedValue = SPEED_HIGH;
                break;
            default:
                speedValue = 0;
                break;
        }

        stepperLeft.setSpeed(speedValue);
        stepperRight.setSpeed(speedValue);

        stepperLeft.enable();
        stepperRight.enable();

        stepperLeft.startContinuous();
        stepperRight.startContinuous();
    } else if (stateMachine.getCurrentState() == StateMachine::STATE_DOOR_OPENING) {
        // Start the door opening sequence if not already moving
        if (!doorActuator.isMoving()) {
            doorActuator.startExtend(); // Uses default time from Configuration.h
        }
    } else if (stateMachine.getCurrentState() == StateMachine::STATE_DOOR_CLOSING) {
        // Start the door closing sequence if not already moving
        if (!doorActuator.isMoving()) {
            doorActuator.startRetract(); // Uses default time from Configuration.h
        }
    } else {
        stepperLeft.stop();
        stepperRight.stop();

        // Only disable motors in IDLE or EMERGENCY states
        if (stateMachine.getCurrentState() == StateMachine::STATE_IDLE ||
            stateMachine.getCurrentState() == StateMachine::STATE_EMERGENCY) {
            stepperLeft.disable();
            stepperRight.disable();
        }
    }

    // Always update the stepper drivers
    stepperLeft.update();
    stepperRight.update();
}

void setup() {
    Serial.begin(9600);
    Serial.println("Swing starting...");

    Wire.setClock(100000);

    // Initialise components
    buzzer.begin();
    display.begin();
    buttons.begin();
    safetyMonitor.begin();
    ultrasonicFront.begin();
    ultrasonicRear.begin();
    pressureSensor.begin();
    stateMachine.begin();
    stepperLeft.begin();
    stepperRight.begin();
    doorActuator.begin();

    // Set initial stepper directions (opposite for swing motion)
    stepperLeft.setDirection(true);   // Clockwise
    stepperRight.setDirection(false); // Counter-clockwise

    // Startup beep
    buzzer.beep(1000, 100);
    delay(100);
    buzzer.beep(1500, 100);

    // Show startup message
    display.showText("Swing Ready");
    delay(1000);
}

void loop() {
    // Check for button presses
    //handleButtons();

    Serial.println("Loop...");

    // Check sensors at regular intervals
    unsigned long currentMillis = millis();
    if (currentMillis - lastSensorCheck >= SENSOR_CHECK_MS) {
        lastSensorCheck = currentMillis;
        Serial.println("Checking sensors");
        safetyMonitor.checkSafety();
        checkSensors();
        checkUltrasonicSensors();
    }

    // Update motor control
    //updateMotors();

    // Update display at regular intervals
    if (currentMillis - lastDisplayUpdate >= DISPLAY_UPDATE_MS) {
        lastDisplayUpdate = currentMillis;
        Serial.println("Updating display");
        updateDisplay();
    }
}
