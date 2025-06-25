// lib/MotorControl/StepperDriver.cpp
#include "StepperDriver.h"
#include "RuntimeConfig.h"
#include "Debug.h"

StepperDriver::StepperDriver(uint8_t in1Pin, uint8_t in2Pin, uint8_t in3Pin, uint8_t in4Pin, int stepsPerRev)
    : _stepper(stepsPerRev, in1Pin, in2Pin, in3Pin, in4Pin),
      _in1Pin(in1Pin),
      _in2Pin(in2Pin),
      _in3Pin(in3Pin),
      _in4Pin(in4Pin),
      _stepsPerRevolution(stepsPerRev),
      _speed(60),              // Default 60 RPM
      _enabled(false),
      _running(false),
      _clockwise(true),
      _swinging(false),
      _swingDirection(true),
      _swingSteps(0),
      _currentPosition(0),     // Start at center
      _returningHome(false),
      _emergencyHalted(false)
{
    // Constructor initialization complete
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

    DEBUG_PRINT("StepperDriver: Initialized with pins ");
    DEBUG_PRINT(_in1Pin); DEBUG_PRINT(", ");
    DEBUG_PRINT(_in2Pin); DEBUG_PRINT(", ");
    DEBUG_PRINT(_in3Pin); DEBUG_PRINT(", ");
    DEBUG_PRINTLN(_in4Pin);
}

void StepperDriver::setSpeed(uint16_t rpm) {
    _speed = rpm;
    _stepper.setSpeed(_speed);
    DEBUG_PRINT("StepperDriver: Speed set to ");
    DEBUG_PRINT(_speed);
    DEBUG_PRINTLN(" RPM");
}

void StepperDriver::setSwingSpeed(uint8_t speedLevel) {
    RuntimeConfig& config = RuntimeConfig::getInstance();

    switch(speedLevel) {
        case 1: // Low
            _stepsPerInterval = config.getSwingSpeedLowSteps();
            break;
        case 2: // Medium
            _stepsPerInterval = config.getSwingSpeedMediumSteps();
            break;
        case 3: // High
            _stepsPerInterval = config.getSwingSpeedHighSteps();
            break;
        default:
            _stepsPerInterval = config.getSwingSpeedLowSteps();
            break;
    }

    DEBUG_PRINT("StepperDriver: Swing speed set to ");
    DEBUG_PRINT(_stepsPerInterval);
    DEBUG_PRINTLN(" steps per interval");
}


void StepperDriver::setDirection(bool clockwise) {
    _clockwise = clockwise;
}

void StepperDriver::enable() {
    _enabled = true;
    DEBUG_PRINTLN("StepperDriver: Enabled");
}

void StepperDriver::disable() {
    _enabled = false;
    _running = false;
    _swinging = false;
    _returningHome = false;
    _emergencyHalted = false;
    setPinsLow();
}

void StepperDriver::startContinuous() {
    if (_enabled && !_swinging && !_emergencyHalted) {
        _running = true;
        DEBUG_PRINT("StepperDriver: Started continuous rotation ");
        DEBUG_PRINTLN(_clockwise ? "clockwise" : "counter-clockwise");
    }
}

void StepperDriver::stop() {
    _running = false;
    _swinging = false;
    _returningHome = false;
    // Note: This is different from emergencyHalt - it allows normal operation to resume
}

void StepperDriver::startSwinging() {
    if (!_enabled) {
        Serial.println("StepperDriver: Cannot start swinging - motor disabled");
        return;
    }

    if (_emergencyHalted) {
        Serial.println("StepperDriver: Cannot start swinging - in emergency halt");
        return;
    }

    RuntimeConfig& config = RuntimeConfig::getInstance();

    // Calculate physics parameters
    _maxSwingSteps = (config.getSwingMaxAngleDegrees() * _stepsPerRevolution) / 360;
    _stepInterval = config.getSwingStepIntervalMs();
    _stepsPerInterval = config.getSwingSpeedLowSteps(); // Start at low speed

    // Initialize swing state
    _swinging = true;
    _running = false;
    _returningHome = false;
    _smoothStopping = false;
    _swingStartTime = millis();
    _lastStepTime = millis();

    // COMPREHENSIVE STARTUP DEBUG
    Serial.println("=== SWING STARTUP DEBUG ===");
    Serial.print("Steps per revolution: "); Serial.println(_stepsPerRevolution);
    Serial.print("Max swing angle: "); Serial.println(config.getSwingMaxAngleDegrees());
    Serial.print("Calculated max swing steps: "); Serial.println(_maxSwingSteps);
    Serial.print("Step interval (ms): "); Serial.println(_stepInterval);
    Serial.print("Steps per interval: "); Serial.println(_stepsPerInterval);
    Serial.print("Swing period (ms): "); Serial.println(config.getSwingPeriodMs());
    Serial.print("Expected swing range: +/-"); Serial.print(_maxSwingSteps); Serial.println(" steps");
    Serial.println("=== END STARTUP DEBUG ===");

    DEBUG_PRINTLN("StepperDriver: Started non-blocking pendulum motion");
}



void StepperDriver::stopSwinging() {
    if (_swinging && !_smoothStopping) {
        _smoothStopping = true;
        _smoothStopStartTime = millis();
        DEBUG_PRINTLN("StepperDriver: Initiated smooth pendulum stop");
    }
}


// Enhanced stop methods
void StepperDriver::emergencyHalt() {
    _swinging = false;
    _running = false;
    _returningHome = false;
    _smoothStopping = false;
    _emergencyHalted = true;

    setPinsLow();
    // Don't move - just stop immediately wherever we are
    Serial.println("StepperDriver: EMERGENCY HALT - holding current position");
}

void StepperDriver::smoothStop() {
    if (_swinging) {
        // Let current swing complete, then stop at center
        _swinging = false;
        _returningHome = true;
        Serial.println("StepperDriver: Smooth stop initiated - will return to center");
    } else if (_running) {
        // For continuous motion, just stop and return home
        _running = false;
        returnHome();
    }
}

void StepperDriver::returnHome() {
    if (_enabled && !_emergencyHalted) {
        _swinging = false;
        _running = false;
        _returningHome = true;
        DEBUG_PRINTLN("StepperDriver: Returning to home position");
    }
}

// Position tracking methods
bool StepperDriver::isAtCenter() const {
    return (_currentPosition == 0);
}

int StepperDriver::getCurrentPosition() const {
    return _currentPosition;
}

float StepperDriver::getCurrentAngle() const {
    return (_currentPosition * 360.0) / _stepsPerRevolution;
}

bool StepperDriver::isSwinging() const {
    return _swinging;
}

void StepperDriver::update() {
    if (!_enabled || _emergencyHalted) return;

    unsigned long currentTime = millis();

    if (_returningHome) {
        // Non-blocking return to home
        if (currentTime - _lastStepTime >= _stepInterval) {
            if (_currentPosition != 0) {
                int stepDirection = (_currentPosition > 0) ? -1 : 1;
                _stepper.step(stepDirection);
                updatePosition(stepDirection);
                _lastStepTime = currentTime;

                if (_currentPosition == 0) {
                    _returningHome = false;
                    DEBUG_PRINTLN("StepperDriver: Reached home position");
                }
            } else {
                _returningHome = false;
            }
        }
    }
    else if (_smoothStopping) {
        // Non-blocking smooth stop with deceleration
        unsigned long stopElapsed = currentTime - _smoothStopStartTime;
        RuntimeConfig& config = RuntimeConfig::getInstance();
        uint16_t stopDuration = config.getSwingSmoothStopMs();

        if (stopElapsed >= stopDuration) {
            // Smooth stop complete, return home
            _smoothStopping = false;
            _swinging = false;
            returnHome();
        } else {
            // Gradual deceleration during smooth stop
            float stopProgress = (float)stopElapsed / stopDuration;
            uint16_t slowInterval = _stepInterval + (uint16_t)(stopProgress * _stepInterval * 2);

            if (currentTime - _lastStepTime >= slowInterval) {
                updateSwingPhysics();
                _lastStepTime = currentTime;
            }
        }
    }
    else if (_swinging) {
        // NON-BLOCKING PENDULUM PHYSICS
        if (currentTime - _lastStepTime >= _stepInterval) {
            updateSwingPhysics();
            _lastStepTime = currentTime;
        }
    }
    else if (_running) {
        // Non-blocking continuous rotation
        if (currentTime - _lastStepTime >= _stepInterval) {
            int stepDirection = _clockwise ? 1 : -1;
            _stepper.step(stepDirection);
            updatePosition(stepDirection);
            _lastStepTime = currentTime;
        }
    }
}


bool StepperDriver::isRunning() const {
    return (_running || _swinging || _returningHome) && _enabled && !_emergencyHalted;
}

void StepperDriver::setPinsLow() {
    digitalWrite(_in1Pin, LOW);
    digitalWrite(_in2Pin, LOW);
    digitalWrite(_in3Pin, LOW);
    digitalWrite(_in4Pin, LOW);
}

void StepperDriver::updatePosition(int steps) {
    _currentPosition += steps;
    // Optional: Add bounds checking or wraparound logic if needed
}

void StepperDriver::updateSwingPhysics() {
    // Calculate current position in swing cycle
    float swingProgress = calculateSwingProgress(millis());

    // Calculate target position using sine wave
    int targetPosition = calculateTargetPosition(swingProgress);

    // Add comprehensive debug output every 500ms
    static unsigned long lastDebugTime = 0;
    if (millis() - lastDebugTime > 500) {
        lastDebugTime = millis();
    }

    // Move toward target position (one step at a time)
    if (_currentPosition != targetPosition) {
        int stepDirection = (targetPosition > _currentPosition) ? 1 : -1;

        // Add debug for actual stepping
        static unsigned long lastStepDebug = 0;
        if (millis() - lastStepDebug > 1000) {
            Serial.println("STEPPING - Direction: ");
            Serial.println(stepDirection);
            lastStepDebug = millis();
        }

        // Take multiple steps based on speed setting
        for (uint8_t i = 0; i < _stepsPerInterval; i++) {
            if (_currentPosition != targetPosition) {
                _stepper.step(stepDirection);
                updatePosition(stepDirection);
            }
        }
    }
}


float StepperDriver::calculateSwingProgress(unsigned long currentTime) {
    RuntimeConfig& config = RuntimeConfig::getInstance();
    unsigned long elapsed = currentTime - _swingStartTime;
    uint16_t period = config.getSwingPeriodMs();

    // Return progress from 0.0 to 1.0 in the swing cycle
    return (float)(elapsed % period) / period;
}

int StepperDriver::calculateTargetPosition(float progress) {
    // Sine wave pendulum motion: position = amplitude * sin(2π * progress)
    float sineValue = calculateSinePosition(progress);
    return (int)(sineValue * _maxSwingSteps);
}

float StepperDriver::calculateSinePosition(float progress) {
    // Calculate how long we've been swinging
    unsigned long swingDuration = millis() - _swingStartTime;

    // Phase-shifted sine wave for proper startup
    // Option A: Earlier push (120° = 2π/3)
    float angle = (2.0 * PI * progress) + (2.0 * PI / 3.0);

    // Option B: Later push (60° = π/3)
    // float angle = (2.0 * PI * progress) + (PI / 3.0);

    // Option C: Even later push (45° = π/4)
    // float angle = (2.0 * PI * progress) + (PI / 4.0);

    float sineValue = sin(angle);

    // Gradually ramp up amplitude over first 3 seconds to build momentum naturally
    if (swingDuration < 2000) {
        float amplitudeRamp = (float)swingDuration / 3000.0f;  // 0 to 1 over 3 seconds
        return sineValue * amplitudeRamp;  // Gradually increase amplitude
    } else {
        return sineValue;  // Full amplitude after momentum is built
    }
}



