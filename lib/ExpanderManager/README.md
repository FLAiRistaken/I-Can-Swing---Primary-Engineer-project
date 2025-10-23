# Expander Manager Module

This module provides a robust interface to the MCP23017 I2C GPIO Expander—crucial for extending the limited digital I/O capacity of the Arduino in the "I Can Swing" system.

## Overview

The MCP23017 chip adds 16 additional GPIO pins over I2C. This allows you to operate many buttons, actuators, or extra peripherals that would otherwise exceed the Arduino's native pin count.

## Functionality

- **Initialisation:** Configures all 16 pins as inputs with pull-up resistors by default, matching typical button wiring.
- **Arduino-like API:** Exposes methods synonymous with standard Arduino pin operations:
  - `pinMode(pin, mode)`
  - `digitalRead(pin)`
  - `digitalWrite(pin, state)`
  - `pullUp(pin, state)`
- **I2C Error Handling:**
  - Continuously checks for I2C bus errors or MCP23017 lockup.
  - Triggers a full hardware/software I2C bus reset and MCP23017 re-initialisation when errors are detected.
  - Ensures stable and continuous operation even in the presence of electrical noise (e.g., from motors or large actuators).

## Usage

- Instantiate the `ExpanderManager` object and call `begin()` during setup.
- Use `pinMode()`, `digitalRead()`, `digitalWrite()`, and `pullUp()` just as you would with normal Arduino pins, only these operate on the expander.
- The error recovery system works transparently—if communication is lost, the manager attempts automatic recovery.

## Key Details

- The default I2C address is `0x20`, but this can be changed via hardware on the MCP23017 or by editing the class.
- Handles recovery from both transient bus glitches and MCP23017 chip lockup scenarios.
- Especially important when using multiple high-current devices (actuators, motors), as these can interfere with I2C communication.

## Integration

- Directly used by `ButtonManager` for reading user input.
- Used by `ActuatorDriver` and `DoorActuatorManager` to manipulate actuators.
- Essential for overall reliable hardware operation.