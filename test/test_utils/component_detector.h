// test/test_utils/component_detector.h
#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <SoftwareSerial.h>
#include "Configuration.h"

class ComponentDetector {
public:
    /**
     * Detect if an ultrasonic sensor is connected
     * @param trigPin The trigger pin number
     * @param echoPin The echo pin number
     * @return true if sensor responds with valid readings
     */
    static bool isUltrasonicAvailable(uint8_t trigPin, uint8_t echoPin) {
        // Setup pins
        pinMode(trigPin, OUTPUT);
        pinMode(echoPin, INPUT);

        // Send trigger pulse
        digitalWrite(trigPin, LOW);
        delayMicroseconds(5);
        digitalWrite(trigPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(trigPin, LOW);

        // Measure response (timeout after 30ms if not connected)
        unsigned long duration = pulseIn(echoPin, HIGH, 30000);

        // Check for valid range (between 2cm and 400cm)
        float distance = (duration * 0.034) / 2.0;
        return (duration > 0 && distance < 400);
    }

    /**
     * Detect if a pressure sensor is connected
     * @param analogPin The analog pin to read from
     * @return true if readings indicate a connected sensor
     */
    static bool isPressureSensorAvailable(uint8_t analogPin) {
        // Take multiple readings to avoid flukes
        int sum = 0;
        for (int i = 0; i < 5; i++) {
            sum += analogRead(analogPin);
            delay(10);
        }
        int avg = sum / 5;

        // Disconnected analog pins usually float or read very high
        return (avg < 1000 && avg >= 0);
    }

    /**
     * Detect if an I2C display is connected
     * @return true if display responds on I2C bus
     */
    static bool isDisplayAvailable() {
        Wire.begin();
        Wire.beginTransmission(0x3C); // Common OLED address
        return (Wire.endTransmission() == 0); // 0 = success
    }

    /**
     * Detect if L298N motor driver and stepper motor are available
     * @param in1Pin IN1 control pin
     * @param in2Pin IN2 control pin
     * @param in3Pin IN3 control pin
     * @param in4Pin IN4 control pin
     * @return true if motor pins are controllable
     */
    static bool isStepperDriverAvailable(uint8_t in1Pin, uint8_t in2Pin,
                                      uint8_t in3Pin, uint8_t in4Pin) {
        // Ensure we can control these pins
        uint8_t pins[] = {in1Pin, in2Pin, in3Pin, in4Pin};

        for (int i = 0; i < 4; i++) {
            pinMode(pins[i], OUTPUT);

            // Try writing and verify no conflicts
            digitalWrite(pins[i], HIGH);
            delayMicroseconds(50);
            if (digitalRead(pins[i]) != HIGH) return false;

            digitalWrite(pins[i], LOW);
            delayMicroseconds(50);
            if (digitalRead(pins[i]) != LOW) return false;
        }

        return true;
    }

    /**
     * Detect if door actuator is available
     * @param fwdPin Forward control pin
     * @param revPin Reverse control pin
     * @return true if actuator pins are controllable
     */
    static bool isActuatorAvailable(uint8_t fwdPin, uint8_t revPin) {
        // Check pin control
        pinMode(fwdPin, OUTPUT);
        pinMode(revPin, OUTPUT);

        // Test forward direction
        digitalWrite(fwdPin, HIGH);
        digitalWrite(revPin, LOW);
        delayMicroseconds(50);
        if (digitalRead(fwdPin) != HIGH) return false;

        // Test reverse direction
        digitalWrite(fwdPin, LOW);
        digitalWrite(revPin, HIGH);
        delayMicroseconds(50);
        if (digitalRead(revPin) != HIGH) return false;

        // Reset pins
        digitalWrite(fwdPin, LOW);
        digitalWrite(revPin, LOW);

        return true;
    }

    /**
     * Detect if voice recognition module is available
     * @param rxPin RX pin for SoftwareSerial
     * @param txPin TX pin for SoftwareSerial
     * @return true if module responds
     */
    static bool isVoiceModuleAvailable(uint8_t rxPin, uint8_t txPin) {
        SoftwareSerial tempSerial(rxPin, txPin);
        tempSerial.begin(9600);

        // Send recognition command header and check for response
        tempSerial.write(0xAA); // Command header byte
        tempSerial.write(0x02); // Length
        tempSerial.write(0x31); // Recognize command
        tempSerial.write((uint8_t)0x00); // Default timeout

        // Wait for response
        unsigned long startTime = millis();
        while (millis() - startTime < 100) {
            if (tempSerial.available() > 0) {
                return true; // Any response means module is connected
            }
        }

        return false;
    }

    /**
     * Detect if buzzer is available
     * @param buzzerPin Buzzer control pin
     * @return true if buzzer pin is controllable
     */
    static bool isBuzzerAvailable(uint8_t buzzerPin) {
        pinMode(buzzerPin, OUTPUT);

        // Check if pin can be controlled
        digitalWrite(buzzerPin, HIGH);
        delayMicroseconds(50);
        if (digitalRead(buzzerPin) != HIGH) return false;

        digitalWrite(buzzerPin, LOW);
        return true;
    }

    /**
     * Detect if a button is available
     * @param buttonPin Button input pin
     * @return true if pin reads expected values
     */
    static bool isButtonAvailable(uint8_t buttonPin) {
        pinMode(buttonPin, INPUT_PULLUP);

        // Button pin should read HIGH when using INPUT_PULLUP and not pressed
        delayMicroseconds(50);
        return (digitalRead(buttonPin) == HIGH);
    }

    /**
     * Check all core components at once
     * @return true if all critical components are available
     */
    static bool areCriticalComponentsAvailable() {
        bool ultrasonic = isUltrasonicAvailable(PIN_ULTRASONIC1_TRIG, PIN_ULTRASONIC1_ECHO);
        bool pressure = isPressureSensorAvailable(PIN_PRESSURE_SENSOR);
        bool motors = isStepperDriverAvailable(PIN_STEPPER1_IN1, PIN_STEPPER1_IN2,
                                            PIN_STEPPER1_IN3, PIN_STEPPER1_IN4);
        return (ultrasonic && pressure && motors);
    }

    /**
     * Print comprehensive component status report to Serial
     */
    static void printComponentStatus() {
        Serial.println(F("===== Component Status Report ====="));

        Serial.print(F("Front Ultrasonic: "));
        Serial.println(isUltrasonicAvailable(PIN_ULTRASONIC1_TRIG, PIN_ULTRASONIC1_ECHO) ? "CONNECTED" : "NOT FOUND");

        Serial.print(F("Rear Ultrasonic: "));
        Serial.println(isUltrasonicAvailable(PIN_ULTRASONIC2_TRIG, PIN_ULTRASONIC2_ECHO) ? "CONNECTED" : "NOT FOUND");

        Serial.print(F("Pressure Sensor: "));
        Serial.println(isPressureSensorAvailable(PIN_PRESSURE_SENSOR) ? "CONNECTED" : "NOT FOUND");

        Serial.print(F("Stepper Motors: "));
        Serial.println(isStepperDriverAvailable(PIN_STEPPER1_IN1, PIN_STEPPER1_IN2,
                                             PIN_STEPPER1_IN3, PIN_STEPPER1_IN4) ? "CONNECTED" : "NOT FOUND");

        Serial.print(F("Door Actuator: "));
        Serial.println(isActuatorAvailable(PIN_ACTUATOR_FWD, PIN_ACTUATOR_REV) ? "CONNECTED" : "NOT FOUND");

        Serial.print(F("Display: "));
        Serial.println(isDisplayAvailable() ? "CONNECTED" : "NOT FOUND");

        Serial.print(F("Voice Module: "));
        Serial.println(isVoiceModuleAvailable(PIN_VOICE_RX, PIN_VOICE_TX) ? "CONNECTED" : "NOT FOUND");

        Serial.print(F("Buzzer: "));
        Serial.println(isBuzzerAvailable(PIN_BUZZER) ? "CONNECTED" : "NOT FOUND");

        Serial.println(F("================================="));
    }
};
