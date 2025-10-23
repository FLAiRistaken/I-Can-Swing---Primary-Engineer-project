# Core System Logic

This directory contains the central architectural components of the "I Can Swing" project.

- **`StateMachine.h`**: Defines the central control unit that manages the system's operational states and logic.
- **`Configuration.h`**: Provides all key compile-time constants including hardware pin definitions and safety thresholds.
- **`Debug.h`**: Utility for standardised debug output across modules.

---

## StateMachine Overview

The `StateMachine` orchestrates the entire system's operation by managing states and responding to events from buttons, voice recognition, safety sensors, and other modules.

### How It Works

The state machine is event-driven. Whenever an event occurs (such as a button press, a sensor trigger, or a system action completion), the event is passed to `StateMachine::processEvent(event)`. The system always gives the highest priority to emergencies, then processes events based on its current state:

1. **Emergency events** cause an immediate transition to `STATE_EMERGENCY`, halting all mechanical operations.
2. **Occupant presence** is tracked with a flag set by pressure sensor events; swing motion will not start unless a user is detected.
3. **State-specific event handling:**
    - **Idle:** Accepts start/speed commands only if a user is present; allows opening/closing doors.
    - **Swinging:** Speed can be changed, motion can be stopped, or system halts immediately on obstacle or absentee detection.
    - **Door Opening / Closing:** Waits for confirmation from actuators, handles door transition completion or interruption requests.
    - **Error:** Awaits an explicit user action to reset.
    - **Emergency:** All non-reset events ignored until the user initiates a reset.
4. **Transition logic:** On state change, performs necessary start/stop actions on motors and actuators, and plays auditory feedback through the buzzer.

---

### States

| State                | Description                                                        |
|----------------------|--------------------------------------------------------------------|
| `STATE_IDLE`         | Swing is stationary, doors closed, ready for commands.             |
| `STATE_SWINGING`     | Swing motion is active and managed.                                |
| `STATE_DOOR_OPENING` | Access doors are opening.                                          |
| `STATE_DOOR_CLOSING` | Access doors are closing.                                          |
| `STATE_ERROR`        | Recoverable error. Requires user to reset system.                  |
| `STATE_EMERGENCY`    | Safety-critical halt. No action possible except manual reset.      |

---

### Events

| Event                   | Description                                                        |
|-------------------------|--------------------------------------------------------------------|
| `EVENT_START_PRESSED`   | User requests swing start (via button/voice/web).                  |
| `EVENT_STOP_PRESSED`    | Swing stop requested.                                              |
| `EVENT_SPEED_UP`/`DOWN` | Increment/decrement speed during swinging.                         |
| `EVENT_SPEED_SET_*`     | Set speed directly (low, medium, high).                            |
| `EVENT_DOOR_OPENED`     | Door completed opening.                                            |
| `EVENT_DOOR_CLOSED`     | Door completed closing.                                            |
| `EVENT_EMERGENCY`       | Triggered by safety monitor or dedicated button.                   |
| `EVENT_PRESSURE_ON/OFF` | User sat down or left seat (pressure sensor).                      |
| `EVENT_OBSTACLE_DETECTED`| Safety monitor detected a nearby object/obstacle.                 |
| `EVENT_ALERT_PRESSED`   | User requested buzzer alert.                                       |
| `EVENT_GIVE_MELODY_PRESSED` | Request "give" melody for accessibility feedback.              |
| ...                     | And other system and user actions as shown in the enum.            |

---

## Notable Implementation Details

- **Immediate Safety:** Emergency events are always processed first, overriding all other logic. The entire system remains in the emergency state until reset.
- **Occupant Safety:** The state machine refuses to start swinging unless the seat is occupied, and instantly halts swing if the user leaves while in motion.
- **Transition Actions:** Each state transition triggers necessary motor/actuator actions and audible feedback (buzzer tones, melodies, etc).
- **Door Operations:** Transitions only when fully open or closed, using events returned by the actuator manager.
- **Error/Emergency Recovery:** Requires explicit user action—a design for safety and traceability.
- **Timeout Handling:** Timeout-based door recovery logic is present but commented out, possibly for tuning or future use.

---

### Integration

- Directs all actuator and swing motor commands.
- Receives and processes input from buttons, voice commands, and sensors.
- Invokes buzzer feedback for user interaction.
- Closely integrated with safety monitoring for proactive error/emergency entry.

---

**Summary:**
The StateMachine forms the decision-making heart of the system, ensuring responsive, safe, and reliable operation across all modes. It offers robust event handling, prioritised safety logic, and clear, maintainable separation between operational states.
