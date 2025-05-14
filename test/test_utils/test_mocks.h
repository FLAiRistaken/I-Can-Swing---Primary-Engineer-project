// test/test_utils/test_mocks.h
#pragma once

#include "UltrasonicSensor.h"
#include "PressureSensor.h"
#include "StepperDriver.h"
#include "ActuatorDriver.h"
#include "VoiceRecognition.h"

// Mock ultrasonic sensor for distance simulation
class MockUltrasonicSensor : public UltrasonicSensor {
public:
    MockUltrasonicSensor(const char* name = "MockSensor")
        : UltrasonicSensor(0, 0, name), _mockDistance(100.0f) {}

    // Override distance measurement
    float measureDistance() override {
        return _mockDistance;
    }

    // Control methods
    void setMockDistance(float distance) {
        _mockDistance = distance;
    }

    // Simulate distance trends
    void simulateApproaching(float startDist, float endDist, int steps) {
        _mockDistance = startDist;
        _distanceStep = (endDist - startDist) / steps;
        _remainingSteps = steps;
    }

    // Update simulation (call in test loop)
    void updateSimulation() {
        if (_remainingSteps > 0) {
            _mockDistance += _distanceStep;
            _remainingSteps--;
        }
    }

private:
    float _mockDistance;
    float _distanceStep = 0;
    int _remainingSteps = 0;
};

// Mock pressure sensor for occupancy simulation
class MockPressureSensor : public PressureSensor {
public:
    MockPressureSensor(const char* name = "MockPressure")
        : PressureSensor(0, 500, name), _mockOccupied(false), _mockRawValue(0) {}

    // Override occupancy detection
    bool isOccupied() override {
        return _mockOccupied;
    }

    // Override raw reading
    int readRawValue() override {
        return _mockRawValue;
    }

    // Control methods
    void setMockOccupied(bool occupied) {
        _mockOccupied = occupied;
        _mockRawValue = occupied ? 800 : 200; // Above/below threshold
    }

    void setMockRawValue(int value) {
        _mockRawValue = value;
        _mockOccupied = (value > getThreshold());
    }

    // Simulate pressure fluctuations
    void simulateFluctuation(int baseValue, int amplitude, int cycles) {
        _baseValue = baseValue;
        _fluctuationAmplitude = amplitude;
        _fluctuationCycles = cycles;
        _currentCycle = 0;
    }

    // Update simulation
    void updateSimulation() {
        if (_fluctuationCycles > 0) {
            float angle = (2 * PI * _currentCycle) / _fluctuationCycles;
            _mockRawValue = _baseValue + sin(angle) * _fluctuationAmplitude;
            _mockOccupied = (_mockRawValue > getThreshold());
            _currentCycle = (_currentCycle + 1) % _fluctuationCycles;
        }
    }

private:
    bool _mockOccupied;
    int _mockRawValue;
    int _baseValue = 0;
    int _fluctuationAmplitude = 0;
    int _fluctuationCycles = 0;
    int _currentCycle = 0;
};

// Mock stepper driver for motor simulation
class MockStepperDriver : public StepperDriver {
public:
    MockStepperDriver(const char* name = "MockStepper")
        : StepperDriver(0, 0, 0, 0), _name(name), _mockRunning(false),
          _mockEnabled(false), _mockSpeed(0), _mockPosition(0) {}

    // Override motor control methods
    void enable() override {
        _mockEnabled = true;
    }

    void disable() override {
        _mockEnabled = false;
        _mockRunning = false;
    }

    void setSpeed(uint16_t rpm) override {
        _mockSpeed = rpm;
    }

    uint16_t getSpeed() override {
        return _mockSpeed;
    }

    void startContinuous() override {
        if (_mockEnabled) {
            _mockRunning = true;
        }
    }

    void stop() override {
        _mockRunning = false;
    }

    void step(int steps) override {
        if (_mockEnabled) {
            _mockPosition += steps;
            _lastStepCount = steps;
        }
    }

    bool isRunning() const override {
        return _mockRunning;
    }

    void update() override {
        if (_mockRunning && _mockEnabled) {
            // Simulate rotation based on speed
            _mockPosition += (_clockwise ? 1 : -1);
        }
    }

    // Additional control and inspection methods
    int getMockPosition() const {
        return _mockPosition;
    }

    int getLastStepCount() const {
        return _lastStepCount;
    }

    bool isMockEnabled() const {
        return _mockEnabled;
    }

    // Simulate motor stall
    void simulateStall() {
        _mockStalled = true;
        _mockSpeed = 0;
    }

    bool isStalled() const {
        return _mockStalled;
    }

private:
    const char* _name;
    bool _mockRunning;
    bool _mockEnabled;
    bool _clockwise = true;
    uint16_t _mockSpeed;
    int _mockPosition;
    int _lastStepCount = 0;
    bool _mockStalled = false;
};

// Mock actuator driver for door control simulation
class MockActuatorDriver : public ActuatorDriver {
public:
    MockActuatorDriver(StateMachine* stateMachine = nullptr, const char* name = "MockActuator")
        : ActuatorDriver(0, 0, stateMachine), _name(name), _mockPosition(0) {}

    // Override control methods
    void startExtend(unsigned long timeMs = DOOR_OPEN_TIME_MS) override {
        _mockDirection = DIRECTION_EXTEND;
        _mockMoving = true;
        _operationStartTime = millis();
        _operationDuration = timeMs;
    }

    void startRetract(unsigned long timeMs = DOOR_OPEN_TIME_MS) override {
        _mockDirection = DIRECTION_RETRACT;
        _mockMoving = true;
        _operationStartTime = millis();
        _operationDuration = timeMs;
    }

    void stop() override {
        _mockDirection = DIRECTION_STOP;
        _mockMoving = false;
    }

    // Update simulation
    void update() {
        ActuatorDriver::update(); // Keep original timeout logic

        // Update position based on direction
        if (_mockMoving) {
            if (_mockDirection == DIRECTION_EXTEND) {
                _mockPosition += _mockSpeed;
                if (_mockPosition >= 100) {
                    _mockPosition = 100; // Fully extended
                }
            } else if (_mockDirection == DIRECTION_RETRACT) {
                _mockPosition -= _mockSpeed;
                if (_mockPosition <= 0) {
                    _mockPosition = 0; // Fully retracted
                }
            }
        }
    }

    // Additional control and inspection methods
    int getMockPosition() const {
        return _mockPosition;
    }

    void setMockSpeed(int speed) {
        _mockSpeed = speed;
    }

    void simulateObstacle() {
        _mockObstacleDetected = true;
    }

    bool isObstacleDetected() const {
        return _mockObstacleDetected;
    }

    void simulateComplete() {
        if (_mockDirection == DIRECTION_EXTEND) {
            _mockPosition = 100;
        } else if (_mockDirection == DIRECTION_RETRACT) {
            _mockPosition = 0;
        }
        _mockMoving = false;
    }

private:
    const char* _name;
    Direction _mockDirection = DIRECTION_STOP;
    bool _mockMoving = false;
    int _mockPosition = 0;  // 0-100 (closed to open)
    int _mockSpeed = 5;     // Units per update
    unsigned long _operationStartTime = 0;
    unsigned long _operationDuration = 0;
    bool _mockObstacleDetected = false;
};

// Mock voice recognition
class MockVoiceRecognition : public VoiceRecognition {
public:
    MockVoiceRecognition(StateMachine* stateMachine = nullptr)
        : VoiceRecognition(0, 0, stateMachine), _mockCommandQueued(false) {}

    // Override method that processes commands
    void update() override {
        if (_mockCommandQueued) {
            handleVoiceCommand(_mockCommand);
            _mockCommandQueued = false;
        }
    }

    // Simulate voice command
    void simulateCommand(Command command) {
        _mockCommand = command;
        _mockCommandQueued = true;
        _commandHistory[_commandCount % MAX_HISTORY] = command;
        _commandCount++;
    }

    // Get command history
    Command getLastCommand() const {
        return _commandCount > 0 ? _commandHistory[(_commandCount - 1) % MAX_HISTORY] : CMD_COUNT;
    }

    int getCommandCount() const {
        return _commandCount;
    }

private:
    static const int MAX_HISTORY = 10;
    bool _mockCommandQueued = false;
    Command _mockCommand;
    Command _commandHistory[MAX_HISTORY];
    int _commandCount = 0;
};
