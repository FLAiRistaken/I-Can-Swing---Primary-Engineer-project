#pragma once
#include <Arduino.h>

// ========================
// Pin Definitions
// ========================

// ---- System Control ----
constexpr uint8_t PIN_BUZZER         = 9;   // Digital pin for buzzer
constexpr uint8_t PIN_EMERGENCY_STOP = 2;   // Interrupt-capable pin

// ---- Stepper Motor Control ----
// L298N Motor Driver Pins for Stepper Control
constexpr uint8_t PIN_STEPPER1_IN1 = 3;
constexpr uint8_t PIN_STEPPER1_IN2 = 4;
constexpr uint8_t PIN_STEPPER1_IN3 = 5;
constexpr uint8_t PIN_STEPPER1_IN4 = 6;

// Second motor if needed
constexpr uint8_t PIN_STEPPER2_IN1 = 6;
constexpr uint8_t PIN_STEPPER2_IN2 = 5;
constexpr uint8_t PIN_STEPPER2_IN3 = 4;
constexpr uint8_t PIN_STEPPER2_IN4 = 3;

// ---- Door Actuator ----
constexpr uint8_t PIN_ACTUATOR_FWD = 7;
constexpr uint8_t PIN_ACTUATOR_REV = 8;

// ---- Buttons (use available digital & analog pins as digital) ----
constexpr uint8_t PIN_BTN_START      = A0;   // D12
constexpr uint8_t PIN_BTN_STOP       = A1;   // D13 (also LED_BUILTIN, avoid using LED at same time)
constexpr uint8_t PIN_BTN_SPEED_UP   = A2;   // D14
constexpr uint8_t PIN_BTN_SPEED_DOWN = A3;   // D15
constexpr uint8_t PIN_BTN_DOOR       = A4;   // D16

// ---- Sensors ----
// Pressure sensor (analog only)
constexpr uint8_t PIN_PRESSURE_SENSOR = A5;  // Analog input

// Ultrasonic Sensor 1 (front, use available digital pins)
constexpr uint8_t PIN_ULTRASONIC1_TRIG = 10;  // Already used for stepper1 enable, if conflict, move to another unused pin
constexpr uint8_t PIN_ULTRASONIC1_ECHO = 11;  // Already used for stepper1 dir, if conflict, move to another unused pin

// If you want a second ultrasonic sensor, use any remaining digital pins (not A4/A5, not used for I2C)
constexpr uint8_t PIN_ULTRASONIC2_TRIG = 12;
constexpr uint8_t PIN_ULTRASONIC2_ECHO = 13;

// ---- I2C Display ----
constexpr uint8_t PIN_DISPLAY_SDA = A4; // Reserved for I2C
constexpr uint8_t PIN_DISPLAY_SCL = A5; // Reserved for I2C

// ---- Voice Recognition Module (SoftwareSerial, pick any free digital pins except D0/D1, A4/A5) ----
constexpr uint8_t PIN_VOICE_RX = 0;  // Example: D0 (if not used elsewhere)
constexpr uint8_t PIN_VOICE_TX = 1;  // Example: D1 (if not used elsewhere)

// ========================
// System Constants
// ========================
constexpr uint16_t SPEED_LOW    = 300;
constexpr uint16_t SPEED_MEDIUM = 500;
constexpr uint16_t SPEED_HIGH   = 700;

constexpr unsigned long DOOR_OPEN_TIME_MS   = 5000;
constexpr unsigned long BUTTON_DEBOUNCE_MS  = 50;
constexpr unsigned long DISPLAY_UPDATE_MS   = 1000;
constexpr unsigned long SENSOR_CHECK_MS     = 100;

// ---- Safety Parameters ----
constexpr int PRESSURE_THRESHOLD    = 500; // Analog threshold for occupancy
constexpr int OBSTACLE_DISTANCE_CM  = 30;  // Default ultrasonic warning threshold
constexpr int CRITICAL_DISTANCE_CM  = 10;  // Emergency stop threshold

