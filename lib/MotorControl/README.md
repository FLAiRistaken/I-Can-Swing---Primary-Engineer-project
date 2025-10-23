# Motor Control Module

This directory contains all the drivers and managers responsible for the physical movement of the swing system. It is divided into two main subsystems: the **swing motion** (controlled by stepper motors) and the **door mechanism** (controlled by linear actuators).

## 1. `StepperDriver`

The `StepperDriver` is the most complex component in this module. It is responsible for creating the smooth, oscillating motion of the swing itself.

### Key Responsibilities
*   **Motion Algorithm:** Implements the "unidirectional pumping" logic. Instead of forcing a rigid, two-way motion, it applies a strong push in one direction and then disengages, allowing the swing's natural momentum to carry it through the rest of the cycle. This method is more energy-efficient and produces a more natural swinging motion.
*   **Speed Control:** Translates high-level speed settings (`LOW`, `MEDIUM`, `HIGH`) from the `StateMachine` into specific step counts and intervals to control the power of the "push".
*   **Position Tracking:** Maintains an internal understanding of its position relative to the centre "home" position.
*   **Safety Halts:** Includes methods for an `emergencyHalt()` (an immediate, abrupt stop) and a `smoothStop()` (completes the current swing before gently stopping at the centre).

### How It Works
1.  When `startSwinging()` is called by the `StateMachine`, the driver enters its swinging mode.
2.  Inside its `update()` loop, it calculates its position within a fixed-period cycle (e.g., an 8-second cycle).
3.  For a small portion of the cycle (e.g., the first 15-20%), it engages the stepper motors to provide a strong forward push.
4.  For the remainder of the cycle, it disengages the motors (`return 0.0f`), allowing the swing to coast on its own momentum.
5.  This process repeats, with each push adding more energy and building the swing's amplitude until it reaches a stable state.

### Known Issues & Tuning
*   **Wiring Order:** The constructor for the Arduino `Stepper` library requires a specific pin order (`in1, in3, in2, in4`) for correct two-phase stepping and maximum torque. It is critical to ensure this is implemented correctly.
*   **Tuning:** The effectiveness of the swing motion is highly dependent on tuning the parameters in `RuntimeConfig` (e.g., `swingPeriodMs`, `swingSpeed...Steps`). These values must be adjusted to match the physical properties (weight, friction) of the swing to achieve optimal momentum.

## 2. `DoorActuatorManager` and `ActuatorDriver`

This pair of classes manages the automated doors of the swing. The architecture separates the high-level logic from the low-level control.

### `ActuatorDriver`
*   **Role:** A low-level driver for a *single* linear actuator.
*   **Functionality:** Provides simple methods to `extend()`, `retract()`, and `stop()` the actuator. It also includes a non-blocking, timed operation mode, where it will automatically stop after a specified duration.
*   **Dependencies:** It operates via the `ExpanderManager`, as the actuator control pins are connected to the MCP23017 I/O expander.

### `DoorActuatorManager`
*   **Role:** A high-level manager that synchronises the movement of the *two* door actuators.
*   **Functionality:** It abstracts the control of the two individual actuators into simple, unified commands: `openDoor()` and `closeDoor()`.
*   **State Management:** It keeps track of the doors' state (`OPENING`, `CLOSING`, `STOPPED`) and notifies the main `StateMachine` via events (`EVENT_DOOR_OPENED`, `EVENT_DOOR_CLOSED`) when an operation is complete. This allows the central system to know when it can proceed to the next state (e.g., start swinging only after the doors are fully closed).

### How They Work Together
1.  The main `StateMachine` calls `doorActuatorManager.openDoor()`.
2.  The `DoorActuatorManager` then calls `actuator.startExtend()` on both of its `ActuatorDriver` instances.
3.  Each `ActuatorDriver` energises the appropriate pins on the I/O expander to move the physical actuators.
4.  The `DoorActuatorManager` monitors the elapsed time. When its internal timeout is reached, it calls `stop()` on both drivers and sends an `EVENT_DOOR_OPENED` event back to the `StateMachine`.

