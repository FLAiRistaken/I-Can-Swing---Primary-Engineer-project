#pragma once
#include <Arduino.h>

// ========================
// Pin Definitions
// ========================

// ---- System Control ----
constexpr uint8_t PIN_BUZZER         = 9;   // Digital pin for buzzer
constexpr uint8_t PIN_EMERGENCY_STOP = 3;   // Interrupt-capable pin

// ---- Stepper Motor Control ----
// L298N Motor Driver Pins for Stepper Control
constexpr uint8_t PIN_SWING_MOTOR_IN1 = 4;
constexpr uint8_t PIN_SWING_MOTOR_IN2 = 5;
constexpr uint8_t PIN_SWING_MOTOR_IN3 = 6;
constexpr uint8_t PIN_SWING_MOTOR_IN4 = 7;

// ---- Door Actuator (ON EXPANDER) ----
constexpr uint8_t PIN_ACTUATOR_FWD = 5;
constexpr uint8_t PIN_ACTUATOR_REV = 6;

// ---- Buttons ----
// NOW USING EXPANSION BOARD - set pins corrorlate to the expander pins not the Arduinos
constexpr uint8_t PIN_BTN_START      = 0;   // D12
constexpr uint8_t PIN_BTN_STOP       = 1;   // D13 (also LED_BUILTIN, avoid using LED at same time)
constexpr uint8_t PIN_BTN_SPEED_UP   = 2;   // D14
constexpr uint8_t PIN_BTN_SPEED_DOWN = 3;   // D15
constexpr uint8_t PIN_BTN_DOOR       = 4;   // D16

// ---- Sensors ----
// Pressure sensor (analog only)
constexpr uint8_t PIN_PRESSURE_SENSOR = A0;  // Analog input

// Ultrasonic Sensor 1 (front, use available digital pins)
constexpr uint8_t PIN_ULTRASONIC1_TRIG = A1;  // Already used for stepper1 enable, if conflict, move to another unused pin
constexpr uint8_t PIN_ULTRASONIC1_ECHO = A2;  // Already used for stepper1 dir, if conflict, move to another unused pin

// If you want a second ultrasonic sensor, use any remaining digital pins (not A4/A5, not used for I2C)
constexpr uint8_t PIN_ULTRASONIC2_TRIG = A3;
constexpr uint8_t PIN_ULTRASONIC2_ECHO = 8;

// ---- I2C Display ----
//constexpr uint8_t PIN_DISPLAY_SDA = A5; // Reserved for I2C
//constexpr uint8_t PIN_DISPLAY_SCL = A4; // Reserved for I2C

// ---- I2C Expansion ----
constexpr uint8_t PIN_I2C_SDA = A4;
constexpr uint8_t PIN_I2C_SCL = A5;

// ---- Voice Recognition Module (SoftwareSerial, pick any free digital pins except D0/D1, A4/A5) ----
constexpr uint8_t PIN_VOICE_RX = 8;  // Example: D8 (if not used elsewhere)
constexpr uint8_t PIN_VOICE_TX = 2;  // Example: D7 (if not used elsewhere)

// ========================
// System Constants
// ========================
constexpr uint16_t SPEED_LOW    = 300;
constexpr uint16_t SPEED_MEDIUM = 500;
constexpr uint16_t SPEED_HIGH   = 700;
constexpr unsigned long DOOR_OPEN_TIME_MS   = 5000;
constexpr unsigned long BUTTON_DEBOUNCE_MS  = 50;
constexpr unsigned long SENSOR_CHECK_MS     = 100;

// ---- Safety Parameters ----
constexpr int PRESSURE_THRESHOLD    = 500; // Analog threshold for occupancy
constexpr int OBSTACLE_DISTANCE_CM  = 30;  // Default ultrasonic warning threshold
constexpr int CRITICAL_DISTANCE_CM  = 10;  // Emergency stop threshold

// ---- WiFi Credentials ----
constexpr char WIFI_SSID[] = "JahPhone";
constexpr char WIFI_PASSWORD[] = "password";

