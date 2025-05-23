// lib/MotorControl/StepperDriver.cpp
#include "StepperDriver.h"
#include <climits>

StepperDriver::StepperDriver(uint8_t in1Pin, uint8_t in2Pin, uint8_t in3Pin, uint8_t in4Pin, int stepsPerRev)
    : _stepper(stepsPerRev, in1Pin, in2Pin, in3Pin, in4Pin),
      _in1Pin(in1Pin),
      _in2Pin(in2Pin),
      _in3Pin(in3Pin),
      _in4Pin(in4Pin),
      _stepsPerRevolution(stepsPerRev),
      _speed(60),  // Default 60 RPM
      _enabled(false),
      _running(false),
      _clockwise(true),
      _targetSteps(0),
      _currentSteps(0),
      _lastStepTime(0),
      _stepInterval(0)
{
    // Initialize with Stepper library
}

void StepperDriver::begin() {
    // Set up the control pins as outputs
    pinMode(_in1Pin, OUTPUT);
    pinMode(_in2Pin, OUTPUT);
    pinMode(_in3Pin, OUTPUT);
    pinMode(_in4Pin, OUTPUT);

    // Set all pins low initially (motor off)
    setPinsLow();

    // Configure the stepper speed
    _stepper.setSpeed(_speed);

    Serial.print("StepperDriver: Initialized with pins ");
    Serial.print(_in1Pin); Serial.print(", ");
    Serial.print(_in2Pin); Serial.print(", ");
    Serial.print(_in3Pin); Serial.print(", ");
    Serial.println(_in4Pin);
}

void StepperDriver::setSpeed(uint16_t rpm) {
    // Set speed in RPM (different from old steps/sec)
    _speed = rpm;
    _stepper.setSpeed(_speed);
    calculateStepInterval();

    Serial.print("StepperDriver: Speed set to ");
    Serial.print(_speed);
    Serial.println(" RPM");
}

void StepperDriver::setDirection(bool clockwise) {
    _clockwise = clockwise;
    // Note: Stepper library handles direction internally
    // based on positive/negative step values
}

void StepperDriver::enable() {
    _enabled = true;
    Serial.println("StepperDriver: Enabled");
}

void StepperDriver::disable() {
    _enabled = false;
    _running = false;
    setPinsLow(); // Set all control pins low to disable motor
    //Serial.println("StepperDriver: Disabled");
}

void StepperDriver::startContinuous() {
    if (_enabled) {
        _running = true;
        _targetSteps = _clockwise ? INT_MAX : INT_MIN;  // Set to essentially infinite steps
        _lastStepTime = micros();
        Serial.print("StepperDriver: Started continuous rotation ");
        Serial.println(_clockwise ? "clockwise" : "counter-clockwise");
    }
}

void StepperDriver::stop() {
    _running = false;
    _targetSteps = 0;
    //Serial.println("StepperDriver: Stopped");
}

void StepperDriver::step(int steps) {
    if (_enabled) {
        // Use the Stepper library's step function directly
        // for specified number of steps
        _stepper.step(steps);
        Serial.print("StepperDriver: Stepped ");
        Serial.print(steps);
        Serial.println(" steps");
    }
}

void StepperDriver::update() {
    if (!_running || !_enabled) {
        return;
    }

    unsigned long currentMicros = micros();

    // Check if it's time for the next step
    if (currentMicros - _lastStepTime >= _stepInterval) {
        // Make one step in the current direction
        int stepDirection = _clockwise ? 1 : -1;
        _stepper.step(stepDirection);
        _currentSteps += stepDirection;
        _lastStepTime = currentMicros;

        // Check if we've reached our target
        if ((_clockwise && _currentSteps >= _targetSteps) ||
            (!_clockwise && _currentSteps <= _targetSteps)) {
            _running = false;
        }
    }
}

bool StepperDriver::isRunning() const {
    return _running && _enabled;
}

void StepperDriver::calculateStepInterval() {
    // Convert RPM to microseconds between steps
    if (_speed > 0) {
        // Steps/minute = RPM * steps per revolution
        // Steps/second = steps/minute / 60
        // Microseconds/step = 1,000,000 / steps/second
        _stepInterval = 60 * 1000000UL / (_speed * _stepsPerRevolution);
    } else {
        _stepInterval = 0;
        _running = false;
    }
}

void StepperDriver::setPinsLow() {
    digitalWrite(_in1Pin, LOW);
    digitalWrite(_in2Pin, LOW);
    digitalWrite(_in3Pin, LOW);
    digitalWrite(_in4Pin, LOW);
}

void StepperDriver::enterTestMode() {
    _testMode = true;
    _currentPosition = 0;
    Serial.println("StepperDriver: Entered test mode");
}

void StepperDriver::exitTestMode() {
    _testMode = false;
    stop();
    Serial.println("StepperDriver: Exited test mode");
}

void StepperDriver::testIndividual(int speed, int steps, bool clockwise) {
    if (!_testMode || !_enabled) {
        Serial.println("StepperDriver: Test mode not active or motor disabled");
        return;
    }

    setSpeed(speed);
    setDirection(clockwise);

    int actualSteps = clockwise ? steps : -steps;
    step(actualSteps);
    _currentPosition += actualSteps;

    Serial.print("StepperDriver: Individual test - Speed: ");
    Serial.print(speed);
    Serial.print(" RPM, Steps: ");
    Serial.print(steps);
    Serial.print(", Direction: ");
    Serial.print(clockwise ? "CW" : "CCW");
    Serial.print(", New position: ");
    Serial.println(_currentPosition);
}

void StepperDriver::startRampTest(int startSpeed, int endSpeed, unsigned long duration) {
    if (!_testMode || !_enabled) {
        Serial.println("StepperDriver: Test mode not active or motor disabled");
        return;
    }

    _rampStartTime = millis();
    _rampCurrentSpeed = startSpeed;
    _rampTargetSpeed = endSpeed;

    setSpeed(startSpeed);
    startContinuous();

    Serial.print("StepperDriver: Ramp test started - ");
    Serial.print(startSpeed);
    Serial.print(" to ");
    Serial.print(endSpeed);
    Serial.print(" RPM over ");
    Serial.print(duration);
    Serial.println(" ms");
}

void StepperDriver::updateRampTest() {
    if (!_testMode || !isRunning()) {
        return;
    }

    unsigned long elapsed = millis() - _rampStartTime;
    unsigned long rampDuration = 10000; // 10 seconds for full ramp

    if (elapsed < rampDuration) {
        // Calculate current speed based on elapsed time
        float progress = (float)elapsed / rampDuration;
        int newSpeed = _rampCurrentSpeed + ((_rampTargetSpeed - _rampCurrentSpeed) * progress);

        if (newSpeed != _speed) {
            setSpeed(newSpeed);
            Serial.print("StepperDriver: Ramp speed updated to ");
            Serial.print(newSpeed);
            Serial.println(" RPM");
        }
    } else {
        // Ramp complete
        setSpeed(_rampTargetSpeed);
        Serial.println("StepperDriver: Ramp test completed");
    }
}

void StepperDriver::testDirection360() {
    if (!_testMode || !_enabled) {
        Serial.println("StepperDriver: Test mode not active or motor disabled");
        return;
    }

    int fullRotation = 200; // 200 steps = 360° for 1.8° per step motor

    Serial.println("StepperDriver: Starting 360° direction test");

    // Rotate 360° clockwise
    setSpeed(300); // Medium speed for direction test
    setDirection(true);
    step(fullRotation);
    _currentPosition += fullRotation;

    delay(1000); // Pause between directions

    // Rotate 360° counter-clockwise (return to start)
    setDirection(false);
    step(fullRotation);
    _currentPosition -= fullRotation;

    Serial.println("StepperDriver: 360° direction test completed");
}

int StepperDriver::getCurrentPosition() const {
    return _currentPosition;
}

void StepperDriver::resetPosition() {
    _currentPosition = 0;
    Serial.println("StepperDriver: Position reset to 0");
}

void StepperDriver::setTestPosition(int position) {
    _currentPosition = position;
    Serial.print("StepperDriver: Position set to ");
    Serial.println(position);
}