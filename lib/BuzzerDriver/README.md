## **BuzzerDriver.h**

**Purpose:**
Defines a class for controlling a piezo buzzer connected to an Arduino pin.

**Public Methods:**
- **`BuzzerDriver(uint8_t pin)`**: Constructor that initializes with a specific pin.
- **`void begin()`**: Configures the pin as an output.
- **`void playTone(uint16_t frequency, uint32_t duration)`**: Plays a single tone at a specified frequency and duration.
- **`void playMelody(const unsigned int frequencies[], const unsigned long durations[], int count)`**: Plays a sequence of tones (melody).
- **`void beep(uint16_t frequency = 1000, uint32_t duration = 100)`**: Plays a short beep (default frequency 1000 Hz).
- **`void stop()`**: Stops any ongoing tone playback.

---

## **BuzzerDriver.cpp**

**Purpose:**
Implements all methods defined in `BuzzerDriver.h`.

**Key Methods:**
1. **`begin()`**:
   - Configures the buzzer pin as an output and ensures no tone is playing initially.

2. **`playTone(uint16_t frequency, uint32_t duration)`**:
   - Uses Arduino's `tone()` function to generate sound at a specific frequency and duration.

3. **`beep(uint16_t frequency, uint32_t duration)`**:
   - Calls `playTone()` with default or provided values for frequency and duration.

4. **`playMelody(const unsigned int frequencies[], const unsigned long durations[], int count)`**:
   - Iterates through arrays of frequencies and durations to play multiple tones sequentially.
   - Adds pauses between notes for clarity.

5. **`stop()`**:
   - Stops sound playback using Arduino's `noTone()` function.
