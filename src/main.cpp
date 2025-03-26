#include <Arduino.h>
#include "BuzzerDriver.h"
#include "DisplayDriver.h"

const int BUZZER_PIN = 8;
BuzzerDriver buzzer(BUZZER_PIN);

DisplayDriver display;

const unsigned int melody[] = {262, 294, 330, 349, 392, 440, 494, 523};
const unsigned long noteDurations[] = {200, 200, 200, 200, 200, 200, 200, 400};

// Variables for display test
unsigned long previousMillis = 0;
const long interval = 1000;  // 1 second


// Function declaration
void displayTest();

void setup() {
  Serial.begin(9600);
  while(!Serial) delay(10);

  Serial.println("Buzzer test");

  buzzer.begin();
  buzzer.beep(1000, 200);

  display.begin();
}

void loop() {
  displayTest();
}

void displayTest() {
  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= interval) {
      previousMillis = currentMillis;

      // Toggle between messages
      static bool showHello = true;
      if (showHello) {
          display.showText("Hello, world!");
      } else {
          display.showText("Goodbye, world!");
      }
      showHello = !showHello;
  }
}