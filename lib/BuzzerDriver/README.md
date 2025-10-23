# Buzzer Module

This module handles the system's piezo buzzer, providing audio feedback for alerts, user actions, and state changes.

## Overview

The `BuzzerDriver` class offers a convenient interface to:
- Play single tones of specified frequency and duration.
- Beep with a fixed or custom tone and length.
- Play multi-tone melodies (including predefined alerts and a "give" melody).
- Stop sound output at any time.

## Functionality

- Initialise the buzzer (set pin mode, silence by default).
- Generate tones using Arduino’s `tone()` for simple, blocking playback.
- Play melodies via a sequence of tones and delays.
- Offer dedicated methods for common use cases:
    - `beep()` – Simple single beep.
    - `playAlertTone()` – High-pitched insistent double beep.
    - `playGiveMelody()` – Recognisable musical sequence for the system’s "give" feedback.
- Stop all output with `stop()` (calls `noTone()`).

## Usage

- Create an instance of `BuzzerDriver`, supplying the physical pin.
- In `setup()`, call `begin()` to initialise the hardware.
- Use methods like `playTone()`, `playAlertTone()`, or `playGiveMelody()` whenever an audible signal is needed.
- Use `stop()` to silence the buzzer immediately if required.

## Example Melodies

- **Alert Tone:** A double beep at 2000Hz for clear attention signals.
- **"Give" Melody:** A hardcoded sequence of musical notes.

## Integration

- Called from the `StateMachine` to provide clear feedback for state changes, errors, and user requests.
- Can be used together with the `DisplayDriver` for accessible, multimodal notifications.

## Future Enhancements

- Refactor for non-blocking (asynchronous) playback to keep the main loop responsive.

