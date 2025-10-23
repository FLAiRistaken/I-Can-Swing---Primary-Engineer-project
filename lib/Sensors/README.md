# Sensors Module

This directory contains the drivers for the physical sensors that give the "I Can Swing" project its awareness of the user and the surrounding environment. These drivers abstract low-level hardware interactions into clean, reusable classes and include important logic for reliability and safety.

## 1. `PressureSensor`

### Purpose
The `PressureSensor` is a critical safety component responsible for determining if a user is seated in the swing. This prevents the swing from operating while empty and allows the `SafetyMonitor` to detect if a user leaves the seat unexpectedly during motion.

### Hardware
*   **Sensor:** A Force-Sensitive Resistor (FSR) placed on the swing's seat.
*   **Connection:** Connected to a single analog pin on the Arduino.

### Implementation Details
The driver is fundamentally simple and relies on polling.
*   The `isOccupied()` method performs a fresh `analogRead()` and compares the raw value against the `PRESSURE_THRESHOLD` constant defined in `Configuration.h`.
*   If `rawValue > threshold`, it returns `true`, indicating the seat is occupied.
*   This straightforward design provides a reliable, real-time check for user presence.

## 2. `UltrasonicSensor`

### Purpose
The `UltrasonicSensor` is used for obstacle detection. The system uses two instances of this driver: one mounted at the front and one at the rear of the swing frame.

### Hardware
*   **Sensor:** HC-SR04 Ultrasonic Distance Sensor.
*   **Connection:** Requires one digital pin for `Trig` and another for `Echo`.

### Implementation Details
This driver is designed for robustness and to avoid disrupting the main program loop.
*   **Dual-Mode Operation:** It offers two ways to measure distance:
    1.  `measureDistance()`: A simple, **blocking** call that waits for the measurement to complete. Used for initialisation or when a quick, one-off reading is needed.
    2.  `startMeasurement()` & `isMeasurementComplete()`: A **non-blocking** pair of methods that allow the main loop to continue running while waiting for the sensor's echo pulse. This is the primary method used by the `SafetyMonitor` for efficient polling.
*   **Timeout & Safe Value:** The driver includes a critical safety feature. If the sensor's echo pulse does not return within a set timeout (indicating no object is in range), it returns a predefined safe value (`500.0f`). This prevents an invalid reading of `0.00` from being misinterpreted as a close obstacle.

## 3. `VoiceRecognition`

### Purpose
The `VoiceRecognition` class provides a hands-free command interface for the user, adding a key accessibility feature. It acts as a high-level wrapper for the underlying `VoiceRecognitionV3` library.

### Hardware
*   **Sensor:** Voice Recognition Module V3.
*   **Connection:** Communicates with the Arduino via a serial connection.

### Implementation Details
The driver is designed to cleanly integrate the external voice module with the project's event-driven architecture.
*   **Robust Initialisation:** The `begin()` method includes a **retry loop**. It attempts to load the trained voice commands up to three times, ensuring the module is properly initialised even if there are transient power-on or communication issues.
*   **Command Mapping:** A predefined `enum` (`CMD_GO`, `CMD_STOP`, etc.) maps the integer indices of the trained voice commands to clear, readable names.
*   **Event-Driven Integration:** The `update()` method polls the hardware module for a recognised command. When a valid command is received, the `handleVoiceCommand()` function translates it into a high-level system `Event` (e.g., receiving `CMD_FASTER` causes it to call `_stateMachine->processEvent(EVENT_SPEED_UP)`). This decouples the core logic from the specifics of the voice hardware.

