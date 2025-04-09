## **StepperDriver.h**

**Purpose:**
Defines a class for controlling stepper motors connected via driver modules (e.g., L298N).

### Public Methods:
1. Constructor:
   - `StepperDriver(uint8_t stepPin, uint8_t dirPin, uint8_t enablePin)`: Initialises with specific pins for control signals.

2. Motor Control:
   - `void begin()`: Configures pins as outputs and initialises motor state variables.
   - `void setSpeed(uint16_t stepsPerSecond)`: Sets motor speed in steps per second.
   - `void setDirection(bool clockwise)`: Sets motor direction (clockwise or counter-clockwise).
   - `void enable() / disable()`: Enables or disables motor operation.

3. Continuous Motion:
   - `void startContinuous() / stop()`: Starts or stops continuous motion without blocking execution.

4. Update Method:
   - `void update()`: Handles non-blocking step generation based on timing intervals.

5. Status Check:
   - `bool isRunning() const`: Returns whether the motor is currently running.
