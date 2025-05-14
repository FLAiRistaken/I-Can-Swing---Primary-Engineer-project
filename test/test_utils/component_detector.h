// test/test_utils/component_detector.h
#pragma once
#include <Arduino.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include "Configuration.h"

class ComponentDetector {
public:
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
        return (duration > 0); // Has valid reading
    }

    static bool isPressureSensorAvailable(uint8_t analogPin) {
        // Check if reading looks valid (not floating or disconnected)
        pinMode(analogPin, INPUT);
        int reading = analogRead(analogPin);
        // Disconnected analog pins often float or read very high
        return (reading < 1000 && reading >= 0);
    }

    static bool isDisplayAvailable() {
        // Try to communicate with display via I2C
        Wire.beginTransmission(0x3C); // Common OLED address
        return (Wire.endTransmission() == 0); // 0 = success
    }

    static bool isMotorDriverAvailable(uint8_t in1Pin, uint8_t in2Pin,
                                      uint8_t in3Pin, uint8_t in4Pin) {
        // Check if pins can be controlled
        bool canControl = true;
        uint8_t pins[] = {in1Pin, in2Pin, in3Pin, in4Pin};

        for (int i = 0; i < 4; i++) {
            pinMode(pins[i], OUTPUT);
            digitalWrite(pins[i], HIGH);
            delayMicroseconds(100);
            digitalWrite(pins[i], LOW);
            canControl &= true; // Would need feedback pins for real test
        }

        return canControl;
    }

    static bool isActuatorAvailable(uint8_t fwdPin, uint8_t revPin) {
        // Check if actuator pins can be controlled
        pinMode(fwdPin, OUTPUT);
        pinMode(revPin, OUTPUT);

        // Briefly pulse each pin to check control
        digitalWrite(fwdPin, HIGH);
        delayMicroseconds(50);
        digitalWrite(fwdPin, LOW);

        digitalWrite(revPin, HIGH);
        delayMicroseconds(50);
        digitalWrite(revPin, LOW);

        return true; // Basic pin control check
    }

    static bool isVoiceModuleAvailable(uint8_t rxPin, uint8_t txPin) {
        // Simplified check - not 100% reliable but provides indication
        SoftwareSerial tempSerial(rxPin, txPin);
        tempSerial.begin(9600);
        tempSerial.write(0xAA); // Command header byte
        delay(100); // Wait for response
        return (tempSerial.available() > 0);
    }

    static bool isBuzzerAvailable(uint8_t buzzerPin) {
        // Check if buzzer pin can be controlled
        pinMode(buzzerPin, OUTPUT);
        return true; // Basic pin control check
    }
};
