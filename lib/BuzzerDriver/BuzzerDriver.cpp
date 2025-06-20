#include "BuzzerDriver.h"

// Notes for Give melody
#define NOTE_E5  659
#define NOTE_G5  784
#define NOTE_A5  880
#define NOTE_B5  988
#define NOTE_C6  1047

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

        // Small pause between notes
        delay(durations[i] + 50);
    }
}

void BuzzerDriver::stop() {
    noTone(_pin);
}

void BuzzerDriver::playAlertTone() {
    // A high-pitched, insistent double beep
    playTone(2500, 200);
    delay(250);
    playTone(2500, 200);
}

void BuzzerDriver::playGiveMelody() {
    const unsigned int giveMelody[] = {
        NOTE_E5, NOTE_G5, NOTE_A5, NOTE_C6,
        NOTE_B5, NOTE_A5, NOTE_G5
    };

    // Note durations in milliseconds
    const unsigned long giveDurations[] = {
        250, 250, 250, 500, // Short, short, short, long
        250, 250, 250      // Short, short, short
    };

    int noteCount = sizeof(giveMelody) / sizeof(giveMelody[0]);

    // Use the existing playMelody helper function
    playMelody(giveMelody, giveDurations, noteCount);
}