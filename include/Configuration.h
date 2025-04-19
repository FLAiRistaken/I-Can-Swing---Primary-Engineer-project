#pragma once

#include <stdint.h>
#include <Arduino.h>

// Pin Definitions
// System Control Pins
constexpr uint8_t PIN_BUZZER = 8;  // Already in use
constexpr uint8_t PIN_EMERGENCY_STOP = 2;  // Use interrupt-capable pin

// Motor Control Pins
constexpr uint8_t PIN_STEPPER1_STEP = 3;
constexpr uint8_t PIN_STEPPER1_DIR = 4;
constexpr uint8_t PIN_STEPPER1_ENABLE = 5;
constexpr uint8_t PIN_STEPPER2_STEP = 6;
constexpr uint8_t PIN_STEPPER2_DIR = 7;
constexpr uint8_t PIN_STEPPER2_ENABLE = 9;

// Door Actuator Pins
constexpr uint8_t PIN_ACTUATOR_FWD = 10;
constexpr uint8_t PIN_ACTUATOR_REV = 11;

// Button Pins
constexpr uint8_t PIN_BTN_START = 22;
constexpr uint8_t PIN_BTN_STOP = 23;
constexpr uint8_t PIN_BTN_SPEED_UP = 24;
constexpr uint8_t PIN_BTN_SPEED_DOWN = 25;
constexpr uint8_t PIN_BTN_DOOR = 26;

// Sensor Pins
constexpr uint8_t PIN_PRESSURE_SENSOR = A0;
constexpr uint8_t PIN_ULTRASONIC1_TRIG = 5;
constexpr uint8_t PIN_ULTRASONIC1_ECHO = 4;
constexpr uint8_t PIN_ULTRASONIC2_TRIG = 32;
constexpr uint8_t PIN_ULTRASONIC2_ECHO = 33;

// Voice Recognition Module
constexpr uint8_t PIN_VOICE_RX = 12;  // Software serial
constexpr uint8_t PIN_VOICE_TX = 13;  // Software serial

// System Constants
// Speed Settings (steps per second)
constexpr uint16_t SPEED_LOW = 300;
constexpr uint16_t SPEED_MEDIUM = 500;
constexpr uint16_t SPEED_HIGH = 700;

// Timing Parameters
constexpr unsigned long DOOR_OPEN_TIME_MS = 5000;  // Time for door to open/close
constexpr unsigned long BUTTON_DEBOUNCE_MS = 50;   // Button debounce time
constexpr unsigned long DISPLAY_UPDATE_MS = 1000;   // Display refresh interval
constexpr unsigned long SENSOR_CHECK_MS = 100;     // Sensor polling interval

// Safety Parameters
constexpr int PRESSURE_THRESHOLD = 500;  // Analog reading threshold for occupancy
constexpr int OBSTACLE_DISTANCE_CM = 30; // Ultrasonic detection threshold
