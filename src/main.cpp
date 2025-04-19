#include <Arduino.h>
#include <Wire.h>
#include "Configuration.h"
#include "BuzzerDriver.h"
#include "DisplayDriver.h"
#include "ButtonManager.h"
#include "StateMachine.h"
#include "StepperDriver.h"
#include "UltrasonicSensor.h"

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

// UsS Distance values
float frontDistance = 0.0;
float rearDistance = 0.0;

// Timing variables
unsigned long lastDisplayUpdate = 0;
unsigned long lastSensorCheck = 0;

// Sensor value
int pressureValue = 0;

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
    sprintf(distanceLine, "Dist F:%0.1f R:%0.1f cm", frontDistance, rearDistance);
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
}

void checkSensors() {
    // Read pressure sensor
    pressureValue = analogRead(PIN_PRESSURE_SENSOR);

    // Check if user is present in the swing
    static bool userPresent = false;
    bool newUserPresent = (pressureValue > PRESSURE_THRESHOLD);

    if (newUserPresent != userPresent) {
        userPresent = newUserPresent;
        stateMachine.processEvent(userPresent ?
                                  StateMachine::EVENT_PRESSURE_ON :
                                  StateMachine::EVENT_PRESSURE_OFF);
    }

    // We'll add ultrasonic sensor code later
}

// Function to check ultrasonic sensors
void checkUltrasonicSensors() {
    // Measure distances
    frontDistance = ultrasonicFront.measureDistance();
    rearDistance = ultrasonicRear.measureDistance();

    // Check for obstacles
    if (frontDistance > 0 && frontDistance < OBSTACLE_DISTANCE_CM) {
        // Obstacle detected in front
        stateMachine.processEvent(StateMachine::EVENT_OBSTACLE_DETECTED);
        Serial.println("Obstical detected front");
        buzzer.beep(1500, 100); // Alert sound
    }

    if (rearDistance > 0 && rearDistance < OBSTACLE_DISTANCE_CM) {
        // Obstacle detected behind
        stateMachine.processEvent(StateMachine::EVENT_OBSTACLE_DETECTED);
        buzzer.beep(1500, 100); // Alert sound
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
    stateMachine.begin();
    stepperLeft.begin();
    stepperRight.begin();

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
        Serial.println("Checking ssensors");
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
