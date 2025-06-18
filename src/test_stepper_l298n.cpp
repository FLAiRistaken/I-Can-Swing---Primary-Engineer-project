// test_stepper_l298n.cpp
#include <Arduino.h>
#include <Stepper.h>
#include "Configuration.h"
#include "BuzzerDriver.h"

// Define steps per revolution for your motor
// NEMA 23 is typically 200 steps/revolution (1.8° per step)
const int STEPS_PER_REVOLUTION = 200;

// L298N control pins (different from your current configuration)
const int IN1_PIN = 4;   // L298N pin IN1
const int IN2_PIN = 5;   // L298N pin IN2
const int IN3_PIN = 6;  // L298N pin IN3
const int IN4_PIN = 7;  // L298N pin IN4

// Initialize the stepper library with the L298N pins
Stepper myStepper(STEPS_PER_REVOLUTION, IN1_PIN, IN2_PIN, IN3_PIN, IN4_PIN);

// Create buzzer instance
BuzzerDriver buzzer(PIN_BUZZER);

void setup() {
  Serial.begin(9600);
  Serial.println("Stepper Motor Test with L298N");
  Serial.println("-----------------------------");

  // Initialize buzzer
  buzzer.begin();

  // Set motor speed (RPM)
  myStepper.setSpeed(40);  // 60 RPM (1 revolution per second)

  // Confirmation beeps
  buzzer.beep(1000, 100);
  delay(100);
  buzzer.beep(1500, 100);

  Serial.println("Test sequence starting...");
}

void loop() {
  // Rotate one full revolution clockwise
  Serial.println("Rotating clockwise 35 steps");
  myStepper.step(35);
  buzzer.beep(800, 100);
  delay(1000);
  Serial.println("Rotating anti-clockwise -105 steps");
  myStepper.step(-105);
  buzzer.beep(800, 100);
  delay(1000);
  Serial.println("Rotating clockwise 105 steps");
  myStepper.step(105);
  buzzer.beep(800, 100);
  delay(1000);
  Serial.println("Rotating anti-clockwise -105 steps");
  myStepper.step(-105);
  buzzer.beep(800, 100);
  delay(1000);
  Serial.println("Rotating clockwise 105 steps");
  myStepper.step(105);
  buzzer.beep(800, 100);
  delay(1000);
  Serial.println("Rotating anti-clockwise -70 steps");
  myStepper.step(-70);
  buzzer.beep(800, 100);

  delay(3000);


  // delay(1000);

  // // Rotate one full revolution counter-clockwise
  // Serial.println("Rotating counter-clockwise 1 revolution");
  // myStepper.step(-STEPS_PER_REVOLUTION);
  // buzzer.beep(1000, 100);

  // delay(1000);

  // // Rotate 90 degrees (quarter turn) clockwise
  // Serial.println("Rotating 90 degrees clockwise");
  // myStepper.step(STEPS_PER_REVOLUTION/4);
  // buzzer.beep(1200, 100);

  // delay(1000);

  // // Return 90 degrees counter-clockwise
  // Serial.println("Returning 90 degrees counter-clockwise");
  // myStepper.step(-STEPS_PER_REVOLUTION/4);
  // buzzer.beep(1200, 100);

  delay(3000);  // Wait 3 seconds before repeating
}
