#include <Arduino.h>
#include <Wire.h>
#include "Configuration.h"
#include "ExpanderManager.h"
#include "RuntimeConfig.h"
#include "BuzzerDriver.h"
#include "ButtonManager.h"
#include "StateMachine.h"
#include "StepperDriver.h"
#include "UltrasonicSensor.h"
#include "PressureSensor.h"
#include "ActuatorDriver.h"
#include "SafetyMonitor.h"
#include "VoiceRecognition.h"
#include "WiFiManager.h"
#include "WebServer.h"

// Create component instances
BuzzerDriver buzzer(PIN_BUZZER);
ExpanderManager expander;
ButtonManager buttons(&expander);
StateMachine stateMachine;
// Create stepper motor
StepperDriver swingMotors(PIN_SWING_MOTOR_IN1, PIN_SWING_MOTOR_IN2, PIN_SWING_MOTOR_IN3, PIN_SWING_MOTOR_IN4);
// Create ultrasonic sensor instances
UltrasonicSensor ultrasonicFront(PIN_ULTRASONIC1_TRIG, PIN_ULTRASONIC1_ECHO, "Front");
UltrasonicSensor ultrasonicRear(PIN_ULTRASONIC2_TRIG, PIN_ULTRASONIC2_ECHO, "Rear");
PressureSensor pressureSensor(PIN_PRESSURE_SENSOR, PRESSURE_THRESHOLD, "BasketSensor");
ActuatorDriver doorActuator(PIN_ACTUATOR_FWD, PIN_ACTUATOR_REV, &stateMachine, &expander);
VoiceRecognition voiceModule(PIN_VOICE_RX, PIN_VOICE_TX, &stateMachine);

// Create SafetyMonitor instance
SafetyMonitor safetyMonitor(&stateMachine, &ultrasonicFront, &ultrasonicRear, &pressureSensor);

WiFiManager wifiManager;
WebServer webServer(&stateMachine, &safetyMonitor);

// UsS Distance values
float frontDistance = 0.0;
float rearDistance = 0.0;

// UsS state variables
bool frontSensorMeasuring = false;
bool rearSensorMeasuring = false;

// Timing variables
unsigned long lastSensorCheck = 0;


void handleButtons() {
    buttons.update();

    // Map button presses to state machine events
    if (buttons.wasPressed(ButtonManager::BTN_START)) {
        Serial.println("DEBUG: Start pressed");
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
        if (digitalRead(PIN_EMERGENCY_STOP) == HIGH) { // Check if physical button is reset (not pressed due to INPUT_PULLUP)
            if (buttons.isPressed(ButtonManager::BTN_STOP)) {
                if (emergencyResetStartTime == 0) {
                    emergencyResetStartTime = millis();
                    buzzer.beep(300, 100);
                } else if (millis() - emergencyResetStartTime > 3000) {
                    stateMachine.processEvent(StateMachine::EVENT_EMERGENCY_RESET);
                    buzzer.beep(700, 100);
                    delay(100);
                    buzzer.beep(1200, 100);
                    emergencyResetStartTime = 0;
                }
            } else {
                emergencyResetStartTime = 0;
            }
        }
    }
}

// Function to check ultrasonic sensors
void checkUltrasonicSensors() {
    // Measure distances
    frontDistance = ultrasonicFront.measureDistance();
    rearDistance = ultrasonicRear.measureDistance();

    // Define thresholds
    RuntimeConfig& config = RuntimeConfig::getInstance();
    float criticalDistance = config.getFrontCriticalDistance(); // Very close - emergency
    float warningDistance = config.getFrontWarningDistance(); // Normal obstacle - error

    // Check for critical proximity (EMERGENCY condition)
    if ((frontDistance > 0 && frontDistance < criticalDistance) ||
        (rearDistance > 0 && rearDistance < criticalDistance)) {
        // Immediate danger detected - trigger emergency
        stateMachine.processEvent(StateMachine::EVENT_EMERGENCY);
        buzzer.playTone(2000, 500); // Urgent alert sound
        Serial.println("CRITICAL: Object extremely close! Emergency triggered.");
        return; // Exit after triggering emergency
    }

    // Check for obstacles (ERROR condition)
    if ((frontDistance > criticalDistance && frontDistance < warningDistance) ||
        (rearDistance > criticalDistance && rearDistance < warningDistance)) {
        // Obstacle detected - trigger error only if not already in error/emergency
        if (stateMachine.getCurrentState() != StateMachine::STATE_ERROR &&
            stateMachine.getCurrentState() != StateMachine::STATE_EMERGENCY) {
            stateMachine.processEvent(StateMachine::EVENT_OBSTACLE_DETECTED);
            buzzer.beep(1500, 100); // Alert sound
            Serial.print("WARNING: Object detected at ");
            Serial.print((frontDistance < warningDistance) ? frontDistance : rearDistance);
            Serial.println(" cm");
        }
    }
}

void updateMotors() {
    swingMotors.update();
}

void setup() {
    Serial.begin(9600);
    Serial.println("Swing starting...");

    Wire.setClock(100000);
    Wire.begin();

    Serial.println("Initialising RuntimeConfig...");
    RuntimeConfig& config = RuntimeConfig::getInstance();
    config.begin();
    Serial.println("RuntimeConfig initialised");

    // Initialise components
    Serial.println("Initialising expander...");
    if (!expander.begin()) {
        Serial.println("FATAL: Expander chip not found. Halting.");
        while(1);
    }
    Serial.println("Expander initialised");
    Serial.println("Initialising buzzer...");
    buzzer.begin();
    Serial.println("Buzzer initialised");
    Serial.println("Initialising buttons...");
    buttons.begin();
    Serial.println("Buttons initialised");
    Serial.println("Initialising safetyMonitor...");
    safetyMonitor.begin();
    Serial.println("safetyMonitor initialised");
    Serial.println("Initialising ultrasonicFront...");
    ultrasonicFront.begin();
    Serial.println("ultrasonicFront initialised");
    delay(50);
    Serial.println("Initialising ultrasonicRear...");
    ultrasonicRear.begin();
    Serial.println("ultrasonicRear initialised");
    delay(50);
    Serial.println("Initialising pressureSensor...");
    pressureSensor.begin();
    Serial.println("pressureSensor initialised");
    Serial.println("Initialising stateMachine...");
    stateMachine.begin();
    Serial.println("stateMachine initialised");
    Serial.println("Initialising swingMotors...");
    swingMotors.begin();
    Serial.println("swingMotors initialised");
    Serial.println("Initialising doorActuator...");
    doorActuator.begin();
    Serial.println("doorActuator initialised");
    Serial.println("Initialising voiceModule");
    voiceModule.begin();
    Serial.println("voiceModule initialised...");
    Serial.println("Initialising WiFi...");
    if (wifiManager.begin(WIFI_SSID, WIFI_PASSWORD)) {
        Serial.println("WiFi connected successfully");
        webServer.begin();
        Serial.println("Web server started");
    } else {
        Serial.println("WiFi connection failed - continuing without web interface");
    }

    stateMachine.setBuzzer(&buzzer);
    stateMachine.setDoorActuator(&doorActuator);
    stateMachine.setDoorTimeout(config.getDoorTimeoutMs());
    stateMachine.setSwingMotor(&swingMotors);

    ultrasonicFront.startMeasurement();
    ultrasonicRear.startMeasurement();

    // Startup beep
    buzzer.beep(1000, 100);
    delay(100);
    buzzer.beep(1500, 100);

    Serial.println("System ready");

    delay(200);
}

void loop() {
    webServer.handleClient();
    // --- All non-blocking updates run on every loop ---
    handleButtons();
    stateMachine.update();
    voiceModule.update(); // Stays commented out for now
    doorActuator.update();
    updateMotors();

    safetyMonitor.update();

    // --- Timed debug printout ---
    static unsigned long lastPrintTime = 0;
    if (millis() - lastPrintTime > 500) {
        lastPrintTime = millis();

        float currentFrontDistance = safetyMonitor.getFrontDistance();
        float currentRearDistance = safetyMonitor.getRearDistance();

        Serial.print("Front: ");
        Serial.print(currentFrontDistance);
        Serial.print(" cm, Rear: ");
        Serial.print(currentRearDistance);
        Serial.println(" cm");
    }

    // --- Other timed events (Unchanged) ---
    static unsigned long lastConfigCheck = 0;
    if (millis() - lastConfigCheck > 30000) {
        RuntimeConfig::getInstance().save();
        lastConfigCheck = millis();
    }
}