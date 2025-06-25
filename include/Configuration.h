#pragma once
#include <Arduino.h>

// ========================
// FINAL PIN ASSIGNMENTS - Arduino UNO R4 WiFi
// ========================

// ---- System Control ----
constexpr uint8_t PIN_BUZZER = 12;          // Moved from pin 13 to avoid upload noise
constexpr uint8_t PIN_EMERGENCY_STOP = 3;   // Interrupt-capable pin (MUST stay here)

// ---- Stepper Motor Control ----
constexpr uint8_t PIN_SWING_MOTOR_IN1 = 4;  // L298N Motor Driver
constexpr uint8_t PIN_SWING_MOTOR_IN2 = 5;  // L298N Motor Driver
constexpr uint8_t PIN_SWING_MOTOR_IN3 = 6;  // L298N Motor Driver
constexpr uint8_t PIN_SWING_MOTOR_IN4 = 7;  // L298N Motor Driver

// ---- Voice Recognition (SoftwareSerial) ----
// TESTED WORKING on Arduino UNO R4 WiFi - pins 2,9 avoid SPI conflicts
constexpr uint8_t PIN_VOICE_RX = 2;          // Working reliably on R4 WiFi
constexpr uint8_t PIN_VOICE_TX = 9;          // Moved from pin 12 to avoid SPI interference

// ---- Ultrasonic Sensors (Stationary on swing frame) ----
constexpr uint8_t PIN_ULTRASONIC1_TRIG = A1; // Front sensor trigger
constexpr uint8_t PIN_ULTRASONIC1_ECHO = A2; // Front sensor echo
constexpr uint8_t PIN_ULTRASONIC2_TRIG = A3; // Rear sensor trigger
constexpr uint8_t PIN_ULTRASONIC2_ECHO = 8;  // Rear sensor echo

// ---- Analog Sensors ----
constexpr uint8_t PIN_PRESSURE_SENSOR = A0;  // User presence detection

// ---- I2C Bus (MCP23017 Expander) ----
constexpr uint8_t PIN_I2C_SDA = A4;          // Reserved for I2C (cannot change)
constexpr uint8_t PIN_I2C_SCL = A5;          // Reserved for I2C (cannot change)

// ---- Door Actuators (ON MCP23017 EXPANDER) ----
constexpr uint8_t PIN_DOOR_ACTUATOR1_FWD = 9;  // Expander pin B1
constexpr uint8_t PIN_DOOR_ACTUATOR1_REV = 10; // Expander pin B2
constexpr uint8_t PIN_DOOR_ACTUATOR2_FWD = 11; // Expander pin B3
constexpr uint8_t PIN_DOOR_ACTUATOR2_REV = 12; // Expander pin B4

// ---- Buttons (ON MCP23017 EXPANDER) ----
// constexpr uint8_t PIN_BTN_START = 0;         // Expander pin 0
constexpr uint8_t PIN_BTN_STOP = 1;          // Expander pin 1
constexpr uint8_t PIN_BTN_SPEED_LOW = 2;     // Expander pin 2
constexpr uint8_t PIN_BTN_SPEED_MEDIUM = 3;  // Expander pin 3
constexpr uint8_t PIN_BTN_SPEED_HIGH = 4;    // Expander pin 4
constexpr uint8_t PIN_BTN_DOOR_OPEN = 5;     // Expander pin 5
constexpr uint8_t PIN_BTN_DOOR_CLOSE = 6;    // Expander pin 6
constexpr uint8_t PIN_BTN_ALERT = 7;         // Expander pin 7
constexpr uint8_t PIN_BTN_GIVE = 8;          // Expander pin B0

// ---- Available Pins (with caveats) ----
// Pin 10: SPI SS - may have interference, use with caution
// Pin 11: SPI COPI - may have interference, use with caution
// Pin 13: SPI SCK + built-in LED - causes noise during upload
// Pins 0,1: USB Serial - disconnect during programming

// ========================
// System Constants
// ========================
constexpr unsigned long DOOR_OPEN_TIME_MS   = 16000;
constexpr unsigned long BUTTON_DEBOUNCE_MS  = 15;
constexpr unsigned long SENSOR_CHECK_MS     = 100;

// ---- Safety Parameters ----
constexpr int PRESSURE_THRESHOLD    = 500; // Analog threshold for occupancy
constexpr int OBSTACLE_DISTANCE_CM  = 30;  // Default ultrasonic warning threshold
constexpr int CRITICAL_DISTANCE_CM  = 10;  // Emergency stop threshold

// ---- WiFi Credentials ----
constexpr char WIFI_SSID[] = "JahPhone";
constexpr char WIFI_PASSWORD[] = "password";

