# Safety Module

This directory contains the `SafetyMonitor`, the central component responsible for ensuring the safe operation of the automated swing system. It acts as the project's vigilant overseer, constantly monitoring the environment and system health.

## 1. Overview

The `SafetyMonitor` is a high-level manager that consolidates data from multiple sensors to make intelligent decisions about the system's safety. It has the authority to interrupt normal operation and force the system into a safe state (`ERROR` or `EMERGENCY_STOP`) by sending high-priority events to the main `StateMachine`.

## 2. Key Responsibilities & Features

*   **Multi-Layered Monitoring:** Continuously assesses several conditions in parallel:
    *   Obstacle proximity (front and rear).
    *   User presence (is someone securely in the swing?).
    *   Motor operation (is the swing motor stalled?).
    *   System responsiveness (via a software watchdog).
*   **Obstacle Detection:** Uses the front and rear ultrasonic sensors to detect objects in the swing's path. It employs two levels of proximity detection:
    *   **Warning:** A non-critical distance that might indicate a potential issue.
    *   **Critical:** An immediate danger zone that triggers an emergency stop.
*   **Dynamic Safety Thresholds:** Intelligently adjusts the sensitivity of the obstacle detection based on the swing's current state. For example, it uses slightly more lenient thresholds when swinging to avoid false positives from the ground or the swing structure itself.
*   **User Presence Verification:** This is a crucial safety feature. The monitor ensures the swing only operates when a user is seated. More importantly, it will trigger an **immediate emergency stop** if it detects that the user has left the seat *while the swing is in motion*.
*   **Unstable Environment Detection:** To prevent false alarms from erratic sensor readings (e.g., a pet running past), the monitor tracks the history of obstacle detections. If it detects an excessive number of rapid changes in a short period, it can pre-emptively trigger an emergency stop.
*   **Motor Stall Detection:** It monitors the motor's activity and speed. If it detects that the motor is commanded to run but is not producing any motion (a stall), it can trigger a safety stop.

## 3. How It Works

1.  The `update()` method of the `SafetyMonitor` is called continuously from the main program loop.
2.  Inside `update()`, it periodically reads data from the ultrasonic and pressure sensors.
3.  It then calls its master checking function, `checkSafety()`.
4.  `checkSafety()` runs each individual safety check (`checkObstacles()`, `checkUserPresence()`, etc.). Each check returns a `SafetyStatus`.
5.  The system determines the **most severe** status reported by any of the checks.
6.  If this new status is more severe than the previous one, the `handleSafetyStatus()` function is called. This function is responsible for sending the correct event (e.g., `EVENT_EMERGENCY`) to the main `StateMachine`.

## 4. Safety Status Levels

The monitor categorises all safety conditions into one of four levels:

| Status             | Meaning                                                                    | `StateMachine` Action                                |
| ------------------ | -------------------------------------------------------------------------- | ---------------------------------------------------- |
| `STATUS_OK`        | All conditions are normal.                                                 | No action.                                           |
| `STATUS_WARNING`   | A minor, non-critical condition has been detected.                         | Currently logs a message; no state change.           |
| `STATUS_ERROR`     | A recoverable error has occurred (e.g., an object is in the warning zone). | Transitions the system to the `STATE_ERROR`.         |
| `STATUS_EMERGENCY` | A critical, non-recoverable event has occurred.                            | Immediately transitions the system to `STATE_EMERGENCY`. |

## 5. Dependencies

The `SafetyMonitor` is a central module that interacts with several other parts of the system:

*   **`StateMachine`**: It receives a pointer to the main state machine so it can send events and check the current system state.
*   **`UltrasonicSensor`**: It depends on two instances (front and rear) to get proximity data.
*   **`PressureSensor`**: It depends on the pressure sensor to determine user presence.
*   **`RuntimeConfig`**: It reads safety thresholds (like warning distances) from the runtime configuration, allowing them to be tuned without recompiling.
