#pragma once

#include <Arduino.h>
#include <U8g2lib.h> // Include the U8g2 library

class DisplayDriver {
public:
    // Constructor
    DisplayDriver();

    // Methods
    // Initialise the display
    void begin();

    // Clear the display buffer
    void clear();

    // Send buffer contents to the display
    void display();

    // Draw text on the display; to the specified x and y coordinates
    void drawText(uint8_t x, uint8_t y, const char* text);

    // Set the font to be used for text drawing
    void setFont(const uint8_t* font);

    // Show text on the display;
    // clears the display, draws the text, and sends the buffer to the display
    void showText(const char* text);

private:
    // U8G2 display object for the SH1106 128x64 display; our particular display
    U8G2_SH1106_128X64_NONAME_F_HW_I2C _display;
    // Pointer to the font to be used for text drawing
    const uint8_t* _font;
};