#include "BuzzerDriver.h"

// Constructor
BuzzerDriver::BuzzerDriver(uint8_t pin) : _pin(pin) {}

// Initialise the buzzer pin
void BuzzerDriver::begin() {
    pinMode(_pin, OUTPUT);
    stop();
}

void BuzzerDriver::playTone(uint16_t frequency, uint32_t duration) {
    tone(_pin, frequency, duration);
}

void BuzzerDriver::beep(uint16_t frequency, uint32_t duration) {
    playTone(frequency, duration);
}

void BuzzerDriver::playMelody(const unsigned int frequencies[], const unsigned long durations[], int count) {
    for (int i = 0; i < count; i++) {
        playTone(frequencies[i], durations[i]);

        // Add a small pause between notes
        delay(durations[i] + 50);
    }
}

void BuzzerDriver::stop() {
    noTone(_pin);
}