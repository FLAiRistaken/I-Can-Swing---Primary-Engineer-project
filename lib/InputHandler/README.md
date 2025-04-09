## **ButtonManager.h**

**Purpose:**
Defines a class for managing multiple button inputs with debouncing and emergency stop handling.

**Public Methods:**
- **`ButtonManager()`**: Constructor initializes internal state variables.
- **`void begin()`**: Configures button pins as inputs with pull-up resistors.
- **`void update()`**: Reads button states and applies debouncing logic.
- **`bool isPressed(Button button)`**: Checks if a button is currently pressed.
- **`bool wasPressed(Button button)`**: Returns true only once per button press (edge detection).
- **Static Method:**
  - `static void emergencyStopISR()`: Interrupt Service Routine for handling emergency stop button presses.
