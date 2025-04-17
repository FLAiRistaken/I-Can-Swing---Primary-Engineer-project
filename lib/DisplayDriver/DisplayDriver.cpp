#include "DisplayDriver.h"
#include <Wire.h>

// Constructor; initialise with no rotation and no reset pin
DisplayDriver::DisplayDriver() : _display(U8G2_R0, U8X8_PIN_NONE) {}

// Setup the display
void DisplayDriver::begin() {
    // Initialize with a more complete reset sequence
    Wire.begin();
    delay(100); // Give the display time to power up

    _display.begin();
    delay(50);  // Wait after initialization

    // Turn display off then on (software reset)
    _display.sendF("c", 0xAE); // Display off
    delay(20);
    _display.sendF("c", 0xAF); // Display on
    delay(20);

    // Configure essential parameters
    _display.sendF("c", 0xA8); // Set multiplex ratio
    _display.sendF("c", 0x3F); // 64 MUX

    _font = u8g2_font_6x10_tf;
    _display.setFont(_font);
    _display.setFontRefHeightExtendedText();
    _display.setFontPosTop();
    _display.setFontMode(1);

    // Force complete refresh
    _display.clearBuffer();
    _display.sendBuffer();
    delay(100);
}

// Clear the display buffer
void DisplayDriver::clear() {
    _display.clearBuffer();
}

// Send the buffer contents to the display
void DisplayDriver::display() {
    //delayMicroseconds(100);
    _display.sendBuffer();
    //delayMicroseconds(100);
}

// Draw text on the display at position (x, y); must be ran after clear() and before display()
void DisplayDriver::drawText(uint8_t x, uint8_t y, const char* text) {
    _display.drawStr(x, y, text);
}

// Set the font to be used for text drawing
void DisplayDriver::setFont(const uint8_t* font) {
    _font = (uint8_t*)font;
    _display.setFont(_font);
}

// Default show text method; clears the display, draws the text, and sends the buffer to the display
void DisplayDriver::showText(const char* text) {
    // Clear the display buffer
    clear();

    // Calculate the x position for centering the text
    // Get the width of the text
    uint16_t textWidth = _display.getStrWidth(text);
    uint8_t x = (128 - textWidth) / 2; // Center the text; 128 is the width of the display

    // Draw the text
    _display.drawStr(x, 32, text);
    // Update display
    _display.sendBuffer();
}