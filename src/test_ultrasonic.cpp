#include <Arduino.h>
#include "Configuration.h"
#include "UltrasonicSensor.h"
#include "BuzzerDriver.h"

// Create ultrasonic sensor instance
UltrasonicSensor ultrasonicFront(PIN_ULTRASONIC1_TRIG, PIN_ULTRASONIC1_ECHO, "Front");
// Create buzzer instance
BuzzerDriver buzzer(PIN_BUZZER);

// Distance value
float distance = 0.0;
// Last time the buzzer was triggered
unsigned long lastBuzzerTime = 0;
// Minimum time between buzzer sounds (ms)
const unsigned long BUZZER_DELAY = 500;

void setup() {
  Serial.begin(9600);

  // Initialize components
  ultrasonicFront.begin();
  buzzer.begin();

  Serial.println("Ultrasonic Sensor Test with Buzzer");
  Serial.println("----------------------------------");
  Serial.println("Will beep when objects are within 30cm");

  // Initial beep to confirm buzzer works
  buzzer.beep(1000, 100);
  delay(100);
  buzzer.beep(1500, 100);
}

void loop() {
  // Measure distance
  distance = ultrasonicFront.measureDistance();

  // Print result
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  unsigned long currentMillis = millis();

  // Check if object is within threshold distance and enough time has passed since last beep
  if (distance > 0 && distance < OBSTACLE_DISTANCE_CM) {
    if (currentMillis - lastBuzzerTime >= BUZZER_DELAY) {
      // Object detected within 30cm - sound the buzzer
      Serial.println("Object detected! Sounding alert...");
      buzzer.beep(1500, 100);
      lastBuzzerTime = currentMillis;
    }
  }

  // Small delay between readings
  delay(250);
}
