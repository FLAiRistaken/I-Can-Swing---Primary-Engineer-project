# Input Handler Module

This module manages all physical user inputs via buttons attached to the MCP23017 I/O expander, providing clean, debounced, and event-driven interactions with the rest of the system.

## ButtonManager

The `ButtonManager` class is responsible for:

- Reading button states from the MCP23017 I/O expander.
- Handling debouncing logic to ensure reliable button presses.
- Tracking button press events, providing both current and one-shot ("was pressed") detection.
- Managing a dedicated emergency stop input with hardware interrupt support for ultra-fast system shutdown.

### Button Enum

Enumerates all physical buttons used by the system, mapped to their hardware pin assignments via the expander:

- `BTN_STOP` – Stops the swing.
- `BTN_SPEED_LOW`, `BTN_SPEED_MEDIUM`, `BTN_SPEED_HIGH` – Set swing speed.
- `BTN_DOOR_OPEN`, `BTN_DOOR_CLOSE` – Control the entry/exit doors.
- `BTN_ALERT`, `BTN_GIVE` – Trigger audio alerts (buzzer/melody).
- `BTN_EMERGENCY` – Emergency stop (hardwired, interrupt-capable pin).

### Core Methods

- `begin()` – Initialises expander pins and sets up emergency stop interrupt.
- `update()` – Polls button states regularly in the main loop, applying debouncing and edge detection.
- `isPressed(Button)` – Returns the instantaneous state of a button (true if currently held down).
- `wasPressed(Button)` – Returns true once per new button press, then resets. Perfect for event-driven logic.
- `emergencyStopISR()` – Interrupt Service Routine, immediately flags an emergency stop regardless of main loop state.

### Usage

- Call `begin()` during setup to initialise hardware.
- Regularly call `update()` in the main loop to keep state up to date.
- Use `isPressed()` or `wasPressed()` to check button statuses where needed.
- Emergency stop is handled asynchronously via interrupt, and always has highest priority for safety.

## Integration

- Works closely with the `StateMachine` to convert button events into high-level state transitions.
- Relies on `ExpanderManager` for robust hardware abstraction and pin management.
- Core to both user control and ensuring safety via fast detection of critical stop requests.

## Notes

- Debounce timing is set by the `BUTTON_DEBOUNCE_MS` constant in `Configuration.h`.
- Internal logic ensures each button press is only registered once for event handling.
- Emergency stop uses a dedicated interrupt to guarantee immediate system response.