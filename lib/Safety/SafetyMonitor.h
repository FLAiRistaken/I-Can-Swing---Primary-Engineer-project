// lib/Safety/SafetyMonitor.h
#pragma once

#include <Arduino.h>
#include "Configuration.h"
#include "StateMachine.h"
#include "UltrasonicSensor.h"
#include "PressureSensor.h"

class RuntimeConfig;

struct CalibrationData {
    int readingCount;
    float minValue;
    float maxValue;
    float average;
    float baseline;
    bool isValid;
};

class SafetyMonitor {
public:
    enum SafetyStatus {
        STATUS_OK,         // No safety concerns
        STATUS_WARNING,    // Minor safety concern, doesn't require state change
        STATUS_ERROR,      // Safety issue requiring ERROR state
        STATUS_EMERGENCY   // Critical safety issue requiring EMERGENCY state
    };

    SafetyMonitor(StateMachine* stateMachine,
                 UltrasonicSensor* frontSensor,
                 UltrasonicSensor* rearSensor,
                 PressureSensor* pressureSensor);

    void begin();

    // Main method to check all safety conditions
    SafetyStatus checkSafety();

    // Individual safety checks
    SafetyStatus checkObstacles();
    SafetyStatus checkUserPresence();
    SafetyStatus checkSystemHealth();  // For future expansion

    void updateMotorStatus(bool isRunning, uint16_t currentSpeed);

    // Get sensor readings for display/logging
    float getFrontDistance() const;
    float getRearDistance() const;
    bool isUserPresent() const;
    const char* getStatusString() const;
    SafetyStatus getCurrentStatus() const;

    // Dynamic thresolds
    float getEffectiveWarningDistance() const;
    float getEffectiveCriticalDistance() const;

    // Calibration
    void enterCalibrationMode(String sensorType);
    void exitCalibrationMode();
    bool saveCalibrationData();
    void resetCalibrationData();
    CalibrationData getCurrentCalibrationData();
    bool updateSensorThresholds(String sensor, float minVal, float maxVal);

    // Getter methods for calibration data
    float getUltrasonicMinThreshold(int sensor);
    float getUltrasonicMaxThreshold(int sensor);
    float getUltrasonicBaseline(int sensor);
    int getPressureThreshold();
    float getPressureBaseline();

    // Test mode functions
    void simulateObstacleDetection(String sensor, float distance);
    void simulateUserDeparture();
    void simulateMotorStall();
    bool isInTestMode() const;
    void enterTestMode();
    void exitTestMode();

private:
    StateMachine* _stateMachine;
    UltrasonicSensor* _frontSensor;
    UltrasonicSensor* _rearSensor;
    PressureSensor* _pressureSensor;

    // Current sensor values
    float _frontDistance;
    float _rearDistance;
    bool _userPresent;
    SafetyStatus _currentStatus;

    // Motor monitoring
    unsigned long _lastMotionCheck;
    uint16_t _previousSpeed;
    uint8_t _stallCount;
    uint8_t _maxStallCount;
    bool _motorActive;

    // Obstacle detection history
    static const uint8_t MAX_OBSTACLE_HISTORY = 10;
    unsigned long _obstacleDetectionTimes[MAX_OBSTACLE_HISTORY];
    uint8_t _obstacleHistoryIndex;

    // Software watchdog
    unsigned long _lastWatchdogReset;
    bool _watchdogEnabled;
    const unsigned long WATCHDOG_TIMEOUT_MS = 5000; // 5 second timeout

    // Handle safety status changes
    void handleSafetyStatus(SafetyStatus newStatus);
    bool detectRapidObstacleChanges();
    SafetyStatus checkMotorOperation();

    // Swing phase tracking
    enum SwingPhase {
        PHASE_FORWARD,     // Moving forward
        PHASE_BACKWARD,    // Moving backward
        PHASE_UNKNOWN      // Initial or unknown position
    };
    SwingPhase _currentSwingPhase;
    unsigned long _lastPhaseChange;
    float _lastFrontDistance;
    float _lastRearDistance;

    // For expected ground detection filtering
    bool isReadingExpectedGround(float distance, float previosuDistance);

    // Calibration
    bool _calibrationMode;
    String _calibratingsensor;
    CalibrationData _currentCalibration;
    float _calibrationReadings[100];
    int _calibrationIndex;
    unsigned long _lastCalibrationReading;

    // Calibration thresholds (stored values)
    float _ultrasonic1MinThreshold;
    float _ultrasonic1MaxThreshold;
    float _ultrasonic1Baseline;
    float _ultrasonic2MinThreshold;
    float _ultrasonic2MaxThreshold;
    float _ultrasonic2Baseline;
    int _pressureThreshold;
    float _pressureBaseline;

    // Safety test web interface
    bool _testModeEnabled;
    float _simulatedFrontDistance;
    float _simulatedRearDistance;
    bool _simulatedUserPresent;
};
