// lib/Safety/SafetyMonitor.cpp
#include "SafetyMonitor.h"
#include "Debug.h"
#include <RuntimeConfig.h>

void onConfigChange(const char* key) {
    DEBUG_PRINT("SafetyMonitor: Config changed - ");
    DEBUG_PRINTLN(key);
}

SafetyMonitor::SafetyMonitor(StateMachine* stateMachine,
                           UltrasonicSensor* frontLeftSensor,
                           UltrasonicSensor* frontRightSensor,
                           PressureSensor* pressureSensor)
    : _stateMachine(stateMachine),
      _frontLeftSensor(frontLeftSensor),
      _frontRightSensor(frontRightSensor),
      _pressureSensor(pressureSensor),
      _frontLeftDistance(500.0f),
      _frontRightDistance(500.0f),
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
      _lastFrontLeftDistance(0.0f),
      _lastFrontRightDistance(0.0f),
      _lastSensorCheck(0),
      _measureFrontLeftSensor(true),
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
    // Don't send pressure events if in emergency state
    if (_stateMachine->getCurrentState() == StateMachine::STATE_EMERGENCY) {
        // Still run safety checks but don't send pressure events
        checkSafety();
        return;
    }

    // --- Hybrid Ultrasonic Sensor Reading ---
    const unsigned long SENSOR_CHECK_INTERVAL = 100;
    if (millis() - _lastSensorCheck > SENSOR_CHECK_INTERVAL) {
        _lastSensorCheck = millis();
        if (_measureFrontLeftSensor) {
            float frontLeftDist = _frontLeftSensor->measureDistance();
            setFrontLeftDistance(frontLeftDist);
        } else {
            float frontRightDist = _frontRightSensor->measureDistance();
            setFrontRightDistance(frontRightDist);
        }
        _measureFrontLeftSensor = !_measureFrontLeftSensor;
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

void SafetyMonitor::setFrontLeftDistance(float distance) {
    _frontLeftDistance = distance;
}

void SafetyMonitor::setFrontRightDistance(float distance) {
    _frontRightDistance = distance;
}

bool SafetyMonitor::detectRapidObstacleChanges() {
    // Record the current obstacle detection
    unsigned long currentTime = millis();

    RuntimeConfig& config = RuntimeConfig::getInstance();


    // Check if distance crosses the warning threshold
    bool obstacleDetected = (_frontLeftDistance > 0 && _frontLeftDistance < config.getFrontLeftWarningDistance()) ||
                            (_frontRightDistance > 0 && _frontRightDistance < config.getFrontRightWarningDistance());

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
    _frontLeftDistance = _frontLeftSensor->measureDistance();

    _frontRightDistance = _frontRightSensor->measureDistance();

    // Store previous readings for phase detection
    static float lastFrontLeftDistance = _frontLeftDistance;
    static float lastFrontRightDistance = _frontRightDistance;

    // --- Position-Aware Filtering ---
    // Track swing phase if in swinging state
    if (_stateMachine->getCurrentState() == StateMachine::STATE_SWINGING) {
        // Track swing phase based on distance changes
        if (_frontLeftDistance > lastFrontLeftDistance + 5.0f) {
            // Distance increasing - swing moving away
            if (_currentSwingPhase != PHASE_BACKWARD) {
                _currentSwingPhase = PHASE_BACKWARD;
                _lastPhaseChange = millis();
            }
        } else if (_frontLeftDistance < lastFrontLeftDistance - 5.0f) {
            // Distance decreasing - swing moving toward
            if (_currentSwingPhase != PHASE_FORWARD) {
                _currentSwingPhase = PHASE_FORWARD;
                _lastPhaseChange = millis();
            }
        }

        // Filter expected ground readings based on swing phase
        if (isReadingExpectedSwing(_frontLeftDistance, lastFrontLeftDistance)) {
            DEBUG_PRINTLN("SafetyMonitor: Filtering expected ground detection on frontLeft sensor");
            _frontLeftDistance = 400.0f; // Set to max range (filtered)
        }

        if (isReadingExpectedSwing(_frontRightDistance, lastFrontRightDistance)) {
            DEBUG_PRINTLN("SafetyMonitor: Filtering expected ground detection on frontRight sensor");
            _frontRightDistance = 400.0f; // Set to max range (filtered)
        }
    }

    // Store current readings for next comparison
    lastFrontLeftDistance = _frontLeftDistance;
    lastFrontRightDistance = _frontRightDistance;

    // --- Dynamic Safety Thresholds ---
    // Get dynamic thresholds based on current state
    float effectiveCritical = getEffectiveCriticalDistance();
    float effectiveWarning = getEffectiveWarningDistance();

    // Check for critical proximity using dynamic threshold
    if ((_frontLeftDistance > 0 && _frontLeftDistance < effectiveCritical) ||
        (_frontRightDistance > 0 && _frontRightDistance < effectiveCritical)) {
        Serial.print("SafetyMonitor: CRITICAL - Object extremely close! Distance: ");
        Serial.print((_frontLeftDistance < effectiveCritical) ? _frontLeftDistance : _frontRightDistance);
        Serial.print(" cm, Threshold: ");
        Serial.print(effectiveCritical);
        Serial.println(" cm");
        return STATUS_EMERGENCY;
    }

    // Check for obstacles using dynamic threshold
    if ((_frontLeftDistance > effectiveCritical && _frontLeftDistance < effectiveWarning) ||
        (_frontRightDistance > effectiveCritical && _frontRightDistance < effectiveWarning)) {
        Serial.print("SafetyMonitor: WARNING - Object detected at ");
        Serial.print((_frontLeftDistance < effectiveWarning) ? _frontLeftDistance : _frontRightDistance);
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

bool SafetyMonitor::isReadingExpectedSwing(float distance, float previousDistance) {
    // Expected swing pattern: periodic detection of swing seat passing through sensor field

    return false;

    if (_stateMachine->getCurrentState() != StateMachine::STATE_SWINGING) {
        return false; // Only filter during swinging
    }

    // Check if this looks like the swing seat detection
    // Swing seat should appear at a consistent distance (e.g., 30-100cm range)
    bool isPossibleSwingSeat = (distance > 20.0f && distance < 150.0f);

    if (!isPossibleSwingSeat) {
        return false; // Outside expected swing seat range
    }

    // Check for periodic pattern based on swing timing
    RuntimeConfig& config = RuntimeConfig::getInstance();
    unsigned long swingPeriod = config.getSwingPeriodMs();
    unsigned long timeSinceSwingStart = millis() - _lastPhaseChange;

    // Calculate expected swing position based on time
    float swingProgress = (float)(timeSinceSwingStart % swingPeriod) / swingPeriod;
    float expectedPhase = sin(2.0 * PI * swingProgress); // -1 to +1

    // If we're detecting something when swing should be in sensor range, it's likely the seat
    bool swingExpectedInRange = (abs(expectedPhase) > 0.3f); // Swing not at center position

    if (swingExpectedInRange && isPossibleSwingSeat) {
        // This detection matches expected swing timing and distance - likely the swing seat
        return true;
    }

    return false; // Doesn't match expected swing pattern - treat as real obstacle
}


float SafetyMonitor::getEffectiveWarningDistance() const {
    RuntimeConfig& config = RuntimeConfig::getInstance();
    float baseDistance = config.getFrontLeftWarningDistance();
    // When swinging, use a smaller threshold to account for ground detection
    if (_stateMachine->getCurrentState() == StateMachine::STATE_SWINGING) {
        // Reduce warning threshold by 30% during swinging
        return baseDistance * 0.7f;
    }
    return baseDistance;
}

float SafetyMonitor::getEffectiveCriticalDistance() const {
    RuntimeConfig& config = RuntimeConfig::getInstance();
    float baseDistance = config.getFrontLeftCriticalDistance();
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

float SafetyMonitor::getFrontLeftDistance() const {
    return _frontLeftDistance;
}

float SafetyMonitor::getFrontRightDistance() const {
    return _frontRightDistance;
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
