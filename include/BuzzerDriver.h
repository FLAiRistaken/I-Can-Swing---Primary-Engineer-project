#pragma once

#include <Arduino.h>

class BuzzerDriver {
public:
    BuzzerDriver(uint8_t pin);

    void begin();
    void playTone(uint16_t frequency, uint32_t duration);
    void playMelody(const unsigned int frequencies[], const unsigned long durations[], int count);
    void beep(uint16_t frequency = 1000, uint32_t duration = 100);
    void stop();

private:
    uint8_t _pin;
};