// lib/Safety/SafetyMonitor.cpp
#include "SafetyMonitor.h"
#include "Debug.h"
#include <RuntimeConfig.h>

void onConfigChange(const char* key) {
    DEBUG_PRINT("SafetyMonitor: Config changed - ");
    DEBUG_PRINTLN(key);
}

SafetyMonitor::SafetyMonitor(StateMachine* stateMachine,
                           UltrasonicSensor* frontSensor,
                           UltrasonicSensor* rearSensor,
                           PressureSensor* pressureSensor)
    : _stateMachine(stateMachine),
      _frontSensor(frontSensor),
      _rearSensor(rearSensor),
      _pressureSensor(pressureSensor),
      _frontDistance(500.0f),
      _rearDistance(500.0f),
      _userPresent(false),
      _currentStatus(STATUS_OK),
      _lastMotionCheck(0),
      _previousSpeed(0),
      _stallCount(0),
      _maxStallCount(3),
      _motorActive(false),
      _obstacleHistoryIndex(0),
      _lastWatchdogReset(0),
      _watchdogEnabled(true),
      _currentSwingPhase(PHASE_UNKNOWN),
      _lastPhaseChange(0),
      _lastFrontDistance(0.0f),
      _lastRearDistance(0.0f),
      _lastSensorCheck(0),
      _measureFrontSensor(true),
      _lastUserPresentState(false) {}

void SafetyMonitor::begin() {
    DEBUG_PRINTLN("SafetyMonitor: Initialized");

    // Register callback for config changes
    RuntimeConfig& config = RuntimeConfig::getInstance();
    config.registerCallback(onConfigChange);

    // Initialize watchdog timer
    _lastWatchdogReset = millis();
    _watchdogEnabled = true;

    // Clear obstacle history
    for (uint8_t i = 0; i < MAX_OBSTACLE_HISTORY; i++) {
        _obstacleDetectionTimes[i] = 0;
    }
    // Initial safety check
    checkSafety();
}

SafetyMonitor::SafetyStatus SafetyMonitor::checkSafety() {
    // Run all safety checks and determine most critical status
    SafetyStatus obstacleStatus = checkObstacles();
    SafetyStatus userStatus = checkUserPresence();
    SafetyStatus motorStatus = checkMotorOperation();
    SafetyStatus systemStatus = checkSystemHealth();

    // Determine most severe status
    SafetyStatus newStatus = STATUS_OK;

    if (obstacleStatus == STATUS_EMERGENCY ||
        userStatus == STATUS_EMERGENCY ||
        motorStatus == STATUS_EMERGENCY ||
        systemStatus == STATUS_EMERGENCY) {
        newStatus = STATUS_EMERGENCY;
    } else if (obstacleStatus == STATUS_ERROR ||
               userStatus == STATUS_ERROR ||
               motorStatus == STATUS_EMERGENCY ||
               systemStatus == STATUS_ERROR) {
        newStatus = STATUS_ERROR;
    } else if (obstacleStatus == STATUS_WARNING ||
               userStatus == STATUS_WARNING ||
               motorStatus == STATUS_EMERGENCY ||
               systemStatus == STATUS_WARNING) {
        newStatus = STATUS_WARNING;
    }

    // Handle status changes if needed
    if (newStatus != _currentStatus) {
        handleSafetyStatus(newStatus);
        _currentStatus = newStatus;
    }

    return _currentStatus;
}

void SafetyMonitor::update() {
    // --- Hybrid Ultrasonic Sensor Reading ---
    const unsigned long SENSOR_CHECK_INTERVAL = 100;
    if (millis() - _lastSensorCheck > SENSOR_CHECK_INTERVAL) {
        _lastSensorCheck = millis();
        if (_measureFrontSensor) {
            float frontDist = _frontSensor->measureDistance();
            setFrontDistance(frontDist);
        } else {
            float rearDist = _rearSensor->measureDistance();
            setRearDistance(rearDist);
        }
        _measureFrontSensor = !_measureFrontSensor;
    }

    // --- User Presence Event Generation ---
    bool currentUserPresent = isUserPresent();
    if (currentUserPresent != _lastUserPresentState) {
        if (currentUserPresent) {
            _stateMachine->processEvent(StateMachine::EVENT_PRESSURE_ON);
        } else {
            _stateMachine->processEvent(StateMachine::EVENT_PRESSURE_OFF);
        }
        _lastUserPresentState = currentUserPresent;
    }

    // --- Run the core safety logic ---
    checkSafety();
}

void SafetyMonitor::setFrontDistance(float distance) {
    _frontDistance = distance;
}

void SafetyMonitor::setRearDistance(float distance) {
    _rearDistance = distance;
}

bool SafetyMonitor::detectRapidObstacleChanges() {
    // Record the current obstacle detection
    unsigned long currentTime = millis();

    RuntimeConfig& config = RuntimeConfig::getInstance();


    // Check if distance crosses the warning threshold
    bool obstacleDetected = (_frontDistance > 0 && _frontDistance < config.getFrontWarningDistance()) ||
                            (_rearDistance > 0 && _rearDistance < config.getRearWarningDistance());

    // Only record changes in obstacle status
    static bool lastObstacleStatus = false;
    if (obstacleDetected != lastObstacleStatus) {
        lastObstacleStatus = obstacleDetected;

        // Record time of change
        _obstacleDetectionTimes[_obstacleHistoryIndex] = currentTime;
        _obstacleHistoryIndex = (_obstacleHistoryIndex + 1) % MAX_OBSTACLE_HISTORY;

        // Count rapid changes in last 3 seconds
        int rapidChanges = 0;
        for (uint8_t i = 0; i < MAX_OBSTACLE_HISTORY; i++) {
            if (_obstacleDetectionTimes[i] > 0 &&
                currentTime - _obstacleDetectionTimes[i] < 3000) {
                rapidChanges++;
            }
        }

        // If more than 5 changes in 3 seconds, consider it unstable
        if (rapidChanges > 5) {
            DEBUG_PRINTLN("SafetyMonitor: Multiple rapid obstacle detections!");
            return true;
        }
    }

    return false;
}

SafetyMonitor::SafetyStatus SafetyMonitor::checkObstacles() {
    // Use simulated values if in test mode
    _frontDistance = _frontSensor->measureDistance();

    _rearDistance = _rearSensor->measureDistance();

    // Store previous readings for phase detection
    static float lastFrontDistance = _frontDistance;
    static float lastRearDistance = _rearDistance;

    // --- Position-Aware Filtering ---
    // Track swing phase if in swinging state
    if (_stateMachine->getCurrentState() == StateMachine::STATE_SWINGING) {
        // Track swing phase based on distance changes
        if (_frontDistance > lastFrontDistance + 5.0f) {
            // Distance increasing - swing moving away
            if (_currentSwingPhase != PHASE_BACKWARD) {
                _currentSwingPhase = PHASE_BACKWARD;
                _lastPhaseChange = millis();
            }
        } else if (_frontDistance < lastFrontDistance - 5.0f) {
            // Distance decreasing - swing moving toward
            if (_currentSwingPhase != PHASE_FORWARD) {
                _currentSwingPhase = PHASE_FORWARD;
                _lastPhaseChange = millis();
            }
        }

        // Filter expected ground readings based on swing phase
        if (isReadingExpectedGround(_frontDistance, lastFrontDistance)) {
            DEBUG_PRINTLN("SafetyMonitor: Filtering expected ground detection on front sensor");
            _frontDistance = 400.0f; // Set to max range (filtered)
        }

        if (isReadingExpectedGround(_rearDistance, lastRearDistance)) {
            DEBUG_PRINTLN("SafetyMonitor: Filtering expected ground detection on rear sensor");
            _rearDistance = 400.0f; // Set to max range (filtered)
        }
    }

    // Store current readings for next comparison
    lastFrontDistance = _frontDistance;
    lastRearDistance = _rearDistance;

    // --- Dynamic Safety Thresholds ---
    // Get dynamic thresholds based on current state
    float effectiveCritical = getEffectiveCriticalDistance();
    float effectiveWarning = getEffectiveWarningDistance();

    // Check for critical proximity using dynamic threshold
    if ((_frontDistance > 0 && _frontDistance < effectiveCritical) ||
        (_rearDistance > 0 && _rearDistance < effectiveCritical)) {
        Serial.print("SafetyMonitor: CRITICAL - Object extremely close! Distance: ");
        Serial.print((_frontDistance < effectiveCritical) ? _frontDistance : _rearDistance);
        Serial.print(" cm, Threshold: ");
        Serial.print(effectiveCritical);
        Serial.println(" cm");
        return STATUS_EMERGENCY;
    }

    // Check for obstacles using dynamic threshold
    if ((_frontDistance > effectiveCritical && _frontDistance < effectiveWarning) ||
        (_rearDistance > effectiveCritical && _rearDistance < effectiveWarning)) {
        Serial.print("SafetyMonitor: WARNING - Object detected at ");
        Serial.print((_frontDistance < effectiveWarning) ? _frontDistance : _rearDistance);
        Serial.print(" cm, Threshold: ");
        Serial.print(effectiveWarning);
        Serial.println(" cm");
        return STATUS_ERROR;
    }

    // Check for rapid obstacle changes (indicates unstable environment)
    if (detectRapidObstacleChanges()) {
        Serial.println("SafetyMonitor: Unstable environment detected! Emergency stop recommended.");
        return STATUS_EMERGENCY;
    }

    return STATUS_OK;
}

bool SafetyMonitor::isReadingExpectedGround(float distance, float previousDistance) {
    // Expected ground pattern: rapid decrease to fixed short distance at bottom of swing
    bool isExpectedPattern = false;

    // If distance suddenly dropped to <40cm during swing
    if (distance < 40.0f && previousDistance > 80.0f &&
        _stateMachine->getCurrentState() == StateMachine::STATE_SWINGING) {

        // If we're in the forward phase (swing moving toward sensor mounting point)
        if (_currentSwingPhase == PHASE_FORWARD) {
            // Likely detecting the ground at bottom of swing arc
            isExpectedPattern = true;
        }
    }

    return isExpectedPattern;
}

float SafetyMonitor::getEffectiveWarningDistance() const {
    RuntimeConfig& config = RuntimeConfig::getInstance();
    float baseDistance = config.getFrontWarningDistance();
    // When swinging, use a smaller threshold to account for ground detection
    if (_stateMachine->getCurrentState() == StateMachine::STATE_SWINGING) {
        // Reduce warning threshold by 30% during swinging
        return baseDistance * 0.7f;
    }
    return baseDistance;
}

float SafetyMonitor::getEffectiveCriticalDistance() const {
    RuntimeConfig& config = RuntimeConfig::getInstance();
    float baseDistance = config.getFrontCriticalDistance();
    // Critical distance is less affected but still adjustable
    if (_stateMachine->getCurrentState() == StateMachine::STATE_SWINGING) {
        // Reduce critical threshold by 10% during swinging
        return baseDistance * 0.9f;
    }
    return baseDistance;
}

SafetyMonitor::SafetyStatus SafetyMonitor::checkUserPresence() {
    // Check if user is present in the swing
    _userPresent = _pressureSensor->isOccupied();

    // If swinging and user suddenly disappears, trigger emergency
    if (!_userPresent &&
        _stateMachine->getCurrentState() == StateMachine::STATE_SWINGING) {
        Serial.println("SafetyMonitor: EMERGENCY - User absence during swinging");
        return STATUS_EMERGENCY;
    }

    return STATUS_OK;
}

SafetyMonitor::SafetyStatus SafetyMonitor::checkSystemHealth() {
    // Check if the system has been running too long without user interaction
    static unsigned long lastActivityTime = millis();
    static unsigned long systemStartTime = millis();
    unsigned long currentTime = millis();

    _lastWatchdogReset = currentTime;

    // Check for system inactivity (in a real system, this would trigger if no reset occurs)
    if (_watchdogEnabled && (currentTime - _lastWatchdogReset > WATCHDOG_TIMEOUT_MS)) {
        Serial.println("SafetyMonitor: WATCHDOG TIMEOUT - System unresponsive!");
        return STATUS_EMERGENCY;
    }

    return STATUS_OK;
}


void SafetyMonitor::handleSafetyStatus(SafetyStatus newStatus) {
    // Avoid redundant state transitions
    StateMachine::State currentState = _stateMachine->getCurrentState();

    switch (newStatus) {
        case STATUS_EMERGENCY:
            if (currentState != StateMachine::STATE_EMERGENCY) {
                Serial.println("SafetyMonitor: Triggering EMERGENCY event");
                _stateMachine->processEvent(StateMachine::EVENT_EMERGENCY);
            }
            break;

        case STATUS_ERROR:
            if (currentState != StateMachine::STATE_ERROR &&
                currentState != StateMachine::STATE_EMERGENCY) {
                Serial.println("SafetyMonitor: Triggering ERROR event");
                _stateMachine->processEvent(StateMachine::EVENT_OBSTACLE_DETECTED);
            }
            break;

        case STATUS_WARNING:
            // Warnings don't change state currently
            DEBUG_PRINTLN("SafetyMonitor: Warning condition - no state change");
            break;

        case STATUS_OK:
            // No actions needed for OK status
            break;
    }
}

void SafetyMonitor::updateMotorStatus(bool isRunning, uint16_t currentSpeed) {
    // Store motor status for monitoring
    _motorActive = isRunning;

    // Check for stall condition (motor active but speed not changing)
    if (isRunning && _previousSpeed > 0 && currentSpeed == 0) {
        _stallCount++;
        DEBUG_PRINTLN("SafetyMonitor: Possible motor stall detected");
    } else {
        // Reset stall counter if speed is changing properly
        if (_stallCount > 0 && currentSpeed > 0) {
            _stallCount = 0;
        }
    }

    _previousSpeed = currentSpeed;
}

SafetyMonitor::SafetyStatus SafetyMonitor::checkMotorOperation() {
    // Only check if motors are supposed to be active
    if (!_motorActive) {
        return STATUS_OK;
    }

    // Check for repeated stall conditions
    if (_stallCount >= _maxStallCount) {
        Serial.println("SafetyMonitor: Motor stall detected! Emergency stop required.");
        _stallCount = 0; // Reset after triggering
        return STATUS_EMERGENCY;
    }

    return STATUS_OK;
}

float SafetyMonitor::getFrontDistance() const {
    return _frontDistance;
}

float SafetyMonitor::getRearDistance() const {
    return _rearDistance;
}

bool SafetyMonitor::isUserPresent() const {
    return _userPresent;
}

SafetyMonitor::SafetyStatus SafetyMonitor::getCurrentStatus() const {
    return _currentStatus;
}

const char* SafetyMonitor::getStatusString() const {
    switch (_currentStatus) {
        case STATUS_OK:
            return "SAFE";
        case STATUS_WARNING:
            return "WARNING";
        case STATUS_ERROR:
            return "ERROR";
        case STATUS_EMERGENCY:
            return "EMERGENCY";
        default:
            return "UNKNOWN";
    }
}
