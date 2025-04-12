#include "DisplayDriver.h"

// Constructor; initialise with no rotation and no reset pin
DisplayDriver::DisplayDriver() : _display(U8G2_R0, U8X8_PIN_NONE) {}

// Setup the display
void DisplayDriver::begin() {
    _display.begin();
    _display.sendF("ca", 0xd5, 0xF0);
    _font = u8g2_font_6x10_tf;
    _display.setFont(_font);
    _display.setFontRefHeightExtendedText();
    _display.setFontPosTop();
    _display.setFontMode(1);
}

// Clear the display buffer
void DisplayDriver::clear() {
    _display.clearBuffer();
}

// Send the buffer contents to the display
void DisplayDriver::display() {
    _display.sendBuffer();
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