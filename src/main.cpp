#include <Arduino.h>
#include <Wire.h>
#include "Debug.h"
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
#include "DoorActuatorManager.h"
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
StepperDriver swingMotors(PIN_SWING_MOTOR_IN1, PIN_SWING_MOTOR_IN2,
                          PIN_SWING_MOTOR_IN3, PIN_SWING_MOTOR_IN4);
// Create ultrasonic sensor instances
UltrasonicSensor ultrasonicFrontLeft(PIN_ULTRASONIC1_TRIG, PIN_ULTRASONIC1_ECHO, "frontLeft");
UltrasonicSensor ultrasonicFrontRight(PIN_ULTRASONIC2_TRIG, PIN_ULTRASONIC2_ECHO, "frontRight");
PressureSensor pressureSensor(PIN_PRESSURE_SENSOR, PRESSURE_THRESHOLD, "BasketSensor");
DoorActuatorManager doorActuator(PIN_DOOR_ACTUATOR1_FWD, PIN_DOOR_ACTUATOR1_REV,
                                 PIN_DOOR_ACTUATOR2_FWD, PIN_DOOR_ACTUATOR2_REV,
                                 &stateMachine, &expander);
VoiceRecognition voiceModule(PIN_VOICE_RX, PIN_VOICE_TX, &stateMachine);

// Create SafetyMonitor instance
SafetyMonitor safetyMonitor(&stateMachine, &ultrasonicFrontLeft,
                            &ultrasonicFrontRight, &pressureSensor);

WiFiManager wifiManager;
WebServer webServer(&stateMachine, &safetyMonitor);

// UsS Distance values
float frontLeftDistance = 0.0;
float frontRightDistance = 0.0;

// UsS state variables
bool frontLeftSensorMeasuring = false;
bool frontRightSensorMeasuring = false;

// Timing variables
unsigned long lastSensorCheck = 0;


void handleButtons() {
    buttons.update();

    // Map button presses to state machine events
    // if (buttons.wasPressed(ButtonManager::BTN_START)) {
    //     DEBUG_PRINTLN("Start pressed");
    //     stateMachine.processEvent(StateMachine::EVENT_START_PRESSED);
    // }

    if (buttons.wasPressed(ButtonManager::BTN_STOP)) {
        DEBUG_PRINTLN("Stop pressed");
        stateMachine.processEvent(StateMachine::EVENT_STOP_PRESSED);
    }

    if (buttons.wasPressed(ButtonManager::BTN_SPEED_LOW)) {
        DEBUG_PRINTLN("Speed LOW button pressed");
        stateMachine.processEvent(StateMachine::EVENT_SPEED_SET_LOW);
    }
    if (buttons.wasPressed(ButtonManager::BTN_SPEED_MEDIUM)) {
        DEBUG_PRINTLN("Speed MEDIUM button pressed");
        stateMachine.processEvent(StateMachine::EVENT_SPEED_SET_MEDIUM);
    }
    if (buttons.wasPressed(ButtonManager::BTN_SPEED_HIGH)) {
        DEBUG_PRINTLN("Speed HIGH button pressed");
        stateMachine.processEvent(StateMachine::EVENT_SPEED_SET_HIGH);
    }

    if (buttons.wasPressed(ButtonManager::BTN_DOOR_OPEN)) {
        DEBUG_PRINTLN("Door OPEN button pressed");
        stateMachine.processEvent(StateMachine::EVENT_DOOR_OPEN_PRESSED);
    }
    if (buttons.wasPressed(ButtonManager::BTN_DOOR_CLOSE)) {
        DEBUG_PRINTLN("Door CLOSE button pressed");
        stateMachine.processEvent(StateMachine::EVENT_DOOR_CLOSE_PRESSED);
    }

    if (buttons.wasPressed(ButtonManager::BTN_ALERT)) {
        DEBUG_PRINTLN("Alert button pressed");
        stateMachine.processEvent(StateMachine::EVENT_ALERT_PRESSED);
    }
    if (buttons.wasPressed(ButtonManager::BTN_GIVE)) {
        DEBUG_PRINTLN("Give melody button pressed");
        stateMachine.processEvent(StateMachine::EVENT_GIVE_MELODY_PRESSED);
    }

    if (buttons.wasPressed(ButtonManager::BTN_EMERGENCY)) {
        DEBUG_PRINTLN("Emergency button pressed");
        stateMachine.processEvent(StateMachine::EVENT_EMERGENCY);
        buzzer.playTone(2000, 1000);  // Emergency alert
    }
    // Handle error state clearing
    if (stateMachine.getCurrentState() == StateMachine::STATE_ERROR) {
        if (buttons.wasPressed(ButtonManager::BTN_STOP)) {
            DEBUG_PRINTLN("ERROR state cleared by STOP button");
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
    frontLeftDistance = ultrasonicFrontLeft.measureDistance();
    frontRightDistance = ultrasonicFrontRight.measureDistance();

    // Define thresholds
    RuntimeConfig& config = RuntimeConfig::getInstance();
    float criticalDistance = config.getFrontLeftCriticalDistance(); // Very close - emergency
    float warningDistance = config.getFrontLeftWarningDistance(); // Normal obstacle - error

    // Check for critical proximity (EMERGENCY condition)
    if ((frontLeftDistance > 0 && frontLeftDistance < criticalDistance) ||
        (frontRightDistance > 0 && frontRightDistance < criticalDistance)) {
        // Immediate danger detected - trigger emergency
        stateMachine.processEvent(StateMachine::EVENT_EMERGENCY);
        buzzer.playTone(2000, 500); // Urgent alert sound
        Serial.println("CRITICAL: Object extremely close! Emergency triggered.");
        return; // Exit after triggering emergency
    }

    // Check for obstacles (ERROR condition)
    if ((frontLeftDistance > criticalDistance && frontLeftDistance < warningDistance) ||
        (frontRightDistance > criticalDistance && frontRightDistance < warningDistance)) {
        // Obstacle detected - trigger error only if not already in error/emergency
        if (stateMachine.getCurrentState() != StateMachine::STATE_ERROR &&
            stateMachine.getCurrentState() != StateMachine::STATE_EMERGENCY) {
            stateMachine.processEvent(StateMachine::EVENT_OBSTACLE_DETECTED);
            buzzer.beep(1500, 100); // Alert sound
            Serial.print("WARNING: Object detected at ");
            Serial.print((frontLeftDistance < warningDistance) ? frontLeftDistance : frontRightDistance);
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

    DEBUG_PRINTLN("Initialising RuntimeConfig...");
    RuntimeConfig& config = RuntimeConfig::getInstance();
    config.begin();
    DEBUG_PRINTLN("RuntimeConfig initialised");

    // Initialise components
    // DEBUG_PRINTLN("Initialising expander...");
    // if (!expander.begin()) {
    //     Serial.println("FATAL: Expander chip not found. Halting.");
    //     while(1);
    // }
    DEBUG_PRINTLN("Expander initialised");
    DEBUG_PRINTLN("Initialising buzzer...");
    buzzer.begin();
    DEBUG_PRINTLN("Buzzer initialised");
    // DEBUG_PRINTLN("Initialising buttons...");
    // buttons.begin();
    // DEBUG_PRINTLN("Buttons initialised");
    DEBUG_PRINTLN("Initialising safetyMonitor...");
    safetyMonitor.begin();
    DEBUG_PRINTLN("safetyMonitor initialised");
    DEBUG_PRINTLN("Initialising ultrasonicFrontLeft...");
    ultrasonicFrontLeft.begin();
    DEBUG_PRINTLN("ultrasonicFrontLeft initialised");
    delay(50);
    DEBUG_PRINTLN("Initialising ultrasonicFrontRight...");
    ultrasonicFrontRight.begin();
    DEBUG_PRINTLN("ultrasonicFrontRight initialised");
    delay(50);
    DEBUG_PRINTLN("Initialising pressureSensor...");
    pressureSensor.begin();
    DEBUG_PRINTLN("pressureSensor initialised");
    DEBUG_PRINTLN("Initialising stateMachine...");
    stateMachine.begin();
    DEBUG_PRINTLN("stateMachine initialised");
    DEBUG_PRINTLN("Initialising swingMotors...");
    swingMotors.begin();
    DEBUG_PRINTLN("swingMotors initialised");
    // DEBUG_PRINTLN("Initialising doorActuator...");
    // doorActuator.begin();
    // DEBUG_PRINTLN("doorActuator initialised");
    // DEBUG_PRINTLN("Initialising voiceModule");
    // voiceModule.begin();
    // DEBUG_PRINTLN("voiceModule initialised...");
    DEBUG_PRINTLN("Initialising WiFi...");
    if (wifiManager.begin(WIFI_SSID, WIFI_PASSWORD)) {
        DEBUG_PRINTLN("WiFi connected successfully");
        webServer.begin();
        DEBUG_PRINTLN("Web server started");
    } else {
        Serial.println("WiFi connection failed - continuing without web interface");
    }

    stateMachine.setBuzzer(&buzzer);
    stateMachine.setDoorActuator(&doorActuator);
    // stateMachine.setDoorTimeout(config.getDoorTimeoutMs());
    stateMachine.setSwingMotor(&swingMotors);

    ultrasonicFrontLeft.startMeasurement();
    ultrasonicFrontRight.startMeasurement();

    // Startup beep
    buzzer.beep(1000, 100);
    delay(100);
    buzzer.beep(1500, 100);

    Serial.println("System ready");
}

void loop() {
    webServer.handleClient();
    // --- All non-blocking updates run on every loop ---
    // handleButtons();
    stateMachine.update();

    // Voice recognition timing control (10Hz update rate for stability)
    static unsigned long lastVoiceCheck = 0;
    if (millis() - lastVoiceCheck > 100) { // Check every 100ms instead of every loop
        voiceModule.update();
        lastVoiceCheck = millis();
    }

    // doorActuator.update();
    updateMotors();

    safetyMonitor.update();

    // --- Timed debug printout ---
    static unsigned long lastPrintTime = 0;
    if (millis() - lastPrintTime > 2000) {
        lastPrintTime = millis();

        float currentFrontLeftDistance = safetyMonitor.getFrontLeftDistance();
        float currentFrontRightDistance = safetyMonitor.getFrontRightDistance();

        DEBUG_PRINT("frontLeft: ");
        DEBUG_PRINT(currentFrontLeftDistance);
        DEBUG_PRINT(" cm, frontRight: ");
        DEBUG_PRINT(currentFrontRightDistance);
        DEBUG_PRINTLN(" cm");
    }

    // --- Other timed events (Unchanged) ---
    static unsigned long lastConfigCheck = 0;
    if (millis() - lastConfigCheck > 30000) {
        RuntimeConfig::getInstance().save();
        lastConfigCheck = millis();
    }
}