## **DisplayDriver.h**

**Purpose:**
This file defines the `DisplayDriver` class, which provides an interface to control an OLED display using the U8g2 library.

**Public Methods:**
- **`DisplayDriver()`**: Constructor initializes the display object.
- **`void begin()`**: Initializes the display hardware and sets default font settings.
- **`void clear()`**: Clears the display buffer.
- **`void display()`**: Sends the contents of the buffer to the display hardware.
- **`void drawText(uint8_t x, uint8_t y, const char* text)`**: Draws text at specified coordinates (x, y).
- **`void setFont(const uint8_t* font)`**: Sets a custom font for text drawing.
- **`void showText(const char* text)`**: Clears the buffer, draws centered text, and sends it to the display.

**Private Members:**
- `_display`: Instance of `U8G2_SH1106_128X64_NONAME_F_HW_I2C`, representing the OLED display.
- `_font`: Pointer to the currently selected font.

---

## **DisplayDriver.cpp**

**Purpose:**
Implements all methods defined in `DisplayDriver.h`.

**Key Methods:**
1. **`begin()`**:
   - Initializes the OLED display using U8g2 library functions.
   - Sets default font to `u8g2_font_6x10_tf`.
   - Configures font height and positioning modes.

2. **`clear()`**:
   - Clears the internal buffer of the OLED display.

3. **`display()`**:
   - Sends buffered content to the physical display.

4. **`drawText(uint8_t x, uint8_t y, const char* text)`**:
   - Draws a string at specified coordinates without clearing or updating the buffer.

5. **`setFont(const uint8_t* font)`**:
   - Changes the current font used for drawing text.

6. **`showText(const char* text)`**:
   - Clears the buffer, calculates centered text position, draws it, and updates the display.
