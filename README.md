# "I Can Swing" - An Advanced Automated Accessible Swing (1/5th Scale Model)

## 1. Project Overview

"I Can Swing" is a 1/5th scale functional model of a sophisticated, automated swing designed for wheelchair users and individuals with disabilities. Controlled by an Arduino UNO R4 WiFi, the project prioritises safety and accessibility, featuring a multi-layered sensor system and several modern control interfaces. The swing's motion is driven by a custom, physics-aware algorithm that builds momentum naturally, mimicking a real-world swing.

This project serves as a comprehensive example of embedded systems design, integrating motor control, state management, sensor fusion, web connectivity, and user interface design for assistive technology.

## 2. Key Features

*   **Advanced Swing Motion:** Utilises a "unidirectional pumping" algorithm to build and maintain momentum realistically, rather than forcing a rigid sine wave.
*   **Comprehensive Safety System:** An independent `SafetyMonitor` uses ultrasonic and pressure sensors to detect obstacles and user presence, with the authority to trigger an immediate emergency stop.
*   **Multi-Modal Control:**
    *   **Physical Buttons:** Control start, stop, speed, and door operation.
    *   **Web Interface:** A built-in web server allows for control and configuration over WiFi.
    *   **Voice Commands:** A dedicated voice recognition module processes spoken commands for hands-free operation.
*   **Modular & Expandable Architecture:** The codebase is organised into discrete, reusable modules (drivers, managers, handlers) managed by a central state machine.
*   **I/O Expansion:** Uses an MCP23017 I/O expander to manage a large number of buttons and actuators, overcoming the pin limitations of the Arduino.
*   **Automated Entry/Exit System:** Controls linear actuators to open and close doors, simulating an accessible entry mechanism.

## 3. Hardware Requirements

*   **Microcontroller:** Arduino UNO R4 WiFi
*   **Motor Driver:** L298N Dual H-Bridge Motor Driver
*   **Swing Motors:** 2x NEMA 17 Stepper Motors (or similar)
*   **I/O Expander:** Adafruit MCP23017 I2C GPIO Expander
*   **Sensors:**
    *   2x HC-SR04 Ultrasonic Sensors (for obstacle detection)
    *   1x FSR (Force-Sensitive Resistor) Pressure Sensor (for user presence)
    *   1x Voice Recognition Module V3
*   **Actuators:** 2x Linear Actuators (for doors)
*   **User Interface:**
    *   Physical Push Buttons
    *   Buzzer for audio feedback
*   **Power Supply:** 12V-18V DC Power Supply (capable of delivering at least 2A)

## 4. Software Dependencies

*   [Adafruit MCP23X17 Library](https://github.com/adafruit/Adafruit-MCP23017-Arduino-Library)

## 5. Project Structure

The project is organised into a modular structure to separate concerns and improve maintainability.

| Directory            | Purpose                                                                                                    |
| -------------------- | ---------------------------------------------------------------------------------------------------------- |
| **`src`**            | Contains the core application logic: `main.cpp` (setup and loop) and `StateMachine.cpp` (central control). |
| **`include`**        | Contains project-wide header files like `Configuration.h` and `StateMachine.h`.                            |
| **`lib`**            | Contains all the modular hardware drivers and software managers.                                           |
| `lib/MotorControl`   | Manages the stepper motors for swing motion and the linear actuators for the doors.                        |
| `lib/Safety`         | Home of the `SafetyMonitor`, which ensures the swing operates safely by monitoring sensors.                |
| `lib/Sensors`        | Contains drivers for all physical sensors: ultrasonic, pressure, and voice recognition.                    |
| `lib/InputHandler`   | Manages user input from the physical buttons.                                                              |
| `lib/ExpanderManager`| Provides an interface to the MCP23017 I/O expander chip.                                                   |
| `lib/Connectivity`   | Handles WiFi connection and hosts the web server for remote control.                                       |
| `lib/Configuration`  | Manages runtime-configurable settings like swing speed and period.                                         |
| `lib/BuzzerDriver`   | Controls the buzzer for providing simple audio feedback.                                                   |
| `lib/DisplayDriver`  | (Placeholder/Future) Intended to control a display for visual feedback.                                    |

## 6. How to Build and Run

1.  **Open the project** in a compatible IDE (e.g., PlatformIO in VSCode).
2.  **Install the required libraries** listed in the "Software Dependencies" section.
3.  **Verify Pin Assignments:** Open `include/Configuration.h` and ensure all pin numbers match your physical hardware wiring.
4.  **Connect the Arduino:** Connect the Arduino UNO R4 WiFi to your computer via USB.
5.  **Build & Upload:** Use your IDE's "Upload" function to compile the code and flash it to the Arduino.

## 7. Current Project Status

**As of 12 September 2025:**

The project is in the **final tuning phase** for the swing motion.

*   **What Works:** All hardware drivers, the state machine, safety systems, button inputs, and web/voice interfaces are functional. The I/O expander is stable.
*   **What We're Working On:** The core challenge is perfecting the swing motion. The current "unidirectional pumping" algorithm shows promise but requires fine-tuning of power, timing, and duration to consistently build and maintain momentum.
*   **Known Issues:** The stepper motor wiring order in `StepperDriver.cpp` has been identified as a likely cause of low torque and needs to be corrected to match the Arduino `Stepper` library standard.

