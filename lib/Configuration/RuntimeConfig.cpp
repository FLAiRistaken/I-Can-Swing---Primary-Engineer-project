// lib/Configuration/RuntimeConfig.cpp
#include "RuntimeConfig.h"
#include "Configuration.h"
#include "Debug.h"

RuntimeConfig& RuntimeConfig::getInstance() {
    static RuntimeConfig instance;
    return instance;
}

void RuntimeConfig::begin() {
    DEBUG_PRINTLN("RuntimeConfig: Initializing...");
    EEPROM.begin();
    _isDirty = false;
    _callbackCount = 0;

    if (!loadFromEEPROM()) {
        DEBUG_PRINTLN("RuntimeConfig: EEPROM load failed - using defaults");
        loadDefaults();
        saveToEEPROM();
    } else if (!isValid()) {
        DEBUG_PRINTLN("RuntimeConfig: EEPROM validation failed - using defaults");
        loadDefaults();
        saveToEEPROM();
    } else {
        DEBUG_PRINTLN("RuntimeConfig: Successfully loaded from EEPROM");
    }
}


void RuntimeConfig::loadDefaults() {
    memset(&_settings, 0, sizeof(Settings));
    _settings.magic = MAGIC_NUMBER;
    _settings.version = CONFIG_VERSION;

    // Load from your existing Configuration.h
    _settings.frontLeftWarningDistance = OBSTACLE_DISTANCE_CM;
    _settings.frontLeftCriticalDistance = CRITICAL_DISTANCE_CM;
    _settings.frontRightWarningDistance = OBSTACLE_DISTANCE_CM;
    _settings.frontRightCriticalDistance = CRITICAL_DISTANCE_CM;
    _settings.pressureThreshold = PRESSURE_THRESHOLD;

    _settings.speedLow = 20;
    _settings.speedMedium = 20;
    _settings.speedHigh = 20;
    _settings.maxSpeed = 90;

    _settings.doorTimeoutMs = DOOR_OPEN_TIME_MS / 1000; // Store in seconds
    _settings.buzzerVolume = 5;
    _settings.safetyCheckInterval = SENSOR_CHECK_MS;

    // Set default flags
    _settings.flags = FLAG_AUDIO_FEEDBACK | FLAG_VOICE_RECOGNITION | FLAG_WATCHDOG_ENABLED;

    // SWING MOTION
    _settings.swingPeriodMs = 3000;
    _settings.swingStepIntervalMs = 5;

    _settings.swingSpeedLowSteps = 116;
    _settings.swingSpeedMediumSteps = 141;
    _settings.swingSpeedHighSteps = 191;
    _settings.swingSmoothStopMs = 2000;

    // PHYSICS
    _settings.pushDurationPercent = 8;
    _settings.pushPowerPercent = 100;

    _settings.checksum = calculateChecksum();
    _isDirty = true;

    DEBUG_PRINTLN("RuntimeConfig: Loaded defaults matching current working config");
}


bool RuntimeConfig::loadFromEEPROM() {
    EEPROM.get(EEPROM_ADDRESS, _settings);
    return (_settings.magic == MAGIC_NUMBER && _settings.version == CONFIG_VERSION);
}

bool RuntimeConfig::saveToEEPROM() {
    if (!_isDirty) {
        DEBUG_PRINTLN("RuntimeConfig: No changes to save");
        return true;
    }

    _settings.checksum = calculateChecksum();

    DEBUG_PRINTLN("RuntimeConfig: Writing settings to EEPROM...");
    EEPROM.put(EEPROM_ADDRESS, _settings);

    // Verify the write succeeded
    Settings verification;
    EEPROM.get(EEPROM_ADDRESS, verification);

    if (verification.magic == _settings.magic &&
        verification.version == _settings.version &&
        verification.checksum == _settings.checksum) {
        _isDirty = false;
        DEBUG_PRINTLN("RuntimeConfig: EEPROM save verified successfully");
        return true;
    } else {
        DEBUG_PRINTLN("RuntimeConfig: EEPROM save verification FAILED!");
        return false;
    }
}


void RuntimeConfig::save() {
    saveToEEPROM();
}

bool RuntimeConfig::isValid() const {
    bool magicOK = (_settings.magic == MAGIC_NUMBER);
    bool versionOK = (_settings.version == CONFIG_VERSION);
    bool checksumOK = (calculateChecksum() == _settings.checksum);
    bool validationOK = validate();

    if (!magicOK) DEBUG_PRINTLN("RuntimeConfig: Invalid magic number");
    if (!versionOK) DEBUG_PRINTLN("RuntimeConfig: Version mismatch");
    if (!checksumOK) DEBUG_PRINTLN("RuntimeConfig: Checksum mismatch");
    if (!validationOK) DEBUG_PRINTLN("RuntimeConfig: Validation failed");

    DEBUG_PRINT("RuntimeConfig: Validation - Magic=");
    DEBUG_PRINT(magicOK ? "OK" : "FAIL");
    DEBUG_PRINT(", Version=");
    DEBUG_PRINT(versionOK ? "OK" : "FAIL");
    DEBUG_PRINT(", Checksum=");
    DEBUG_PRINT(checksumOK ? "OK" : "FAIL");
    DEBUG_PRINT(", Values=");
    DEBUG_PRINTLN(validationOK ? "OK" : "FAIL");

    return (magicOK && versionOK && checksumOK && validationOK);
}


uint8_t RuntimeConfig::calculateChecksum() const {
    uint8_t checksum = 0;
    const uint8_t* data = (const uint8_t*)&_settings;

    // Calculate checksum for all data except checksum field
    for (size_t i = 0; i < sizeof(Settings); i++) {
        if (i != offsetof(Settings, checksum)) {
            checksum ^= data[i];
        }
    }

    return checksum;
}

bool RuntimeConfig::validate() const {
    // Safety Configuration Validation
    if (_settings.frontLeftWarningDistance <= _settings.frontLeftCriticalDistance ||
        _settings.frontRightWarningDistance <= _settings.frontRightCriticalDistance ||
        _settings.frontLeftWarningDistance < 0 || _settings.frontLeftWarningDistance > 500 ||
        _settings.frontLeftCriticalDistance < 0 || _settings.frontLeftCriticalDistance > 500 ||
        _settings.frontRightWarningDistance < 0 || _settings.frontRightWarningDistance > 500 ||
        _settings.frontRightCriticalDistance < 0 || _settings.frontRightCriticalDistance > 500) {
        DEBUG_PRINTLN("RuntimeConfig: Safety distance validation failed");
        return false;
    }

    // Motor Speed Validation (Allow equal values)
    if (_settings.speedLow < 5 || _settings.speedLow > 200 ||
        _settings.speedMedium < 5 || _settings.speedMedium > 200 ||
        _settings.speedHigh < 5 || _settings.speedHigh > 200 ||
        _settings.maxSpeed < 5 || _settings.maxSpeed > 200) {
        DEBUG_PRINTLN("RuntimeConfig: Motor speed validation failed");
        return false;
    }

    // Swing Motion Parameters Validation
    if (_settings.swingPeriodMs < 1000 || _settings.swingPeriodMs > 10000 ||
        _settings.swingStepIntervalMs < 1 || _settings.swingStepIntervalMs > 100 ||
        _settings.swingMaxAngleDegrees < 10 || _settings.swingMaxAngleDegrees > 180 ||
        _settings.swingSpeedLowSteps < 10 || _settings.swingSpeedLowSteps > 255 ||
        _settings.swingSpeedMediumSteps < 10 || _settings.swingSpeedMediumSteps > 255 ||
        _settings.swingSpeedHighSteps < 10 || _settings.swingSpeedHighSteps > 300 ||
        _settings.swingSmoothStopMs < 500 || _settings.swingSmoothStopMs > 10000) {
        DEBUG_PRINTLN("RuntimeConfig: Swing motion validation failed");
        return false;
    }

    // Physics Parameters Validation
    if (_settings.pushDurationPercent < 5 || _settings.pushDurationPercent > 80 ||
        _settings.pushPowerPercent < 50 || _settings.pushPowerPercent > 100) {
        DEBUG_PRINTLN("RuntimeConfig: Push parameters validation failed");
        return false;
    }

    // System Configuration Validation
    if (_settings.pressureThreshold < 50 || _settings.pressureThreshold > 1000 ||
        _settings.doorTimeoutMs < 5 || _settings.doorTimeoutMs > 120 ||  // Stored in seconds
        _settings.buzzerVolume > 10 ||
        _settings.safetyCheckInterval < 100 || _settings.safetyCheckInterval > 10000) {
        DEBUG_PRINTLN("RuntimeConfig: System configuration validation failed");
        return false;
    }

    DEBUG_PRINTLN("RuntimeConfig: All validation checks passed");
    return true;
}


// Simple setters with validation
bool RuntimeConfig::setWarningDistance(float value) {
    if (value < 5.0f || value > 200.0f || value <= _settings.frontLeftCriticalDistance) {
        DEBUG_PRINTLN("RuntimeConfig: Warning distance out of range (5-200cm) or <= critical distance");
        return false;
    }

    // Set both frontLeft and frontRight warning distances to same value
    _settings.frontLeftWarningDistance = value;
    _settings.frontRightWarningDistance = value;
    _isDirty = true;
    notifyCallbacks("warningDistance");
    DEBUG_PRINT("RuntimeConfig: Warning distance updated to ");
    DEBUG_PRINTLN(value);
    return true;
}


bool RuntimeConfig::setCriticalDistance(float value) {
    if (value < 1.0f || value > 50.0f || value >= _settings.frontLeftWarningDistance) {
        DEBUG_PRINTLN("RuntimeConfig: Critical distance out of range or >= warning distance");
        return false;
    }

    // Set both frontLeft and frontRight critical distances to same value
    _settings.frontLeftCriticalDistance = value;
    _settings.frontRightCriticalDistance = value;
    _isDirty = true;
    notifyCallbacks("criticalDistance");
    DEBUG_PRINT("RuntimeConfig: Critical distance updated to ");
    DEBUG_PRINTLN(value);
    return true;
}

bool RuntimeConfig::setPressureThreshold(uint16_t value) {
    if (value < 100 || value > 900) {
        return false;
    }

    _settings.pressureThreshold = value;
    _isDirty = true;
    notifyCallbacks("pressureThreshold");
    return true;
}

bool RuntimeConfig::setSpeedLow(uint16_t value) {
    if (value < 20 || value > 50) {
        return false;
    }

    _settings.speedLow = value;
    _isDirty = true;
    notifyCallbacks("speedLow");
    return true;
}

bool RuntimeConfig::setSpeedMedium(uint16_t value) {
    if (value < 20 || value > 70) {
        DEBUG_PRINTLN("RuntimeConfig: Medium speed out of range or invalid order");
        return false;
    }

    _settings.speedMedium = value;
    _isDirty = true;
    notifyCallbacks("speedMedium");
    DEBUG_PRINT("RuntimeConfig: Medium speed updated to ");
    DEBUG_PRINTLN(value);
    return true;
}

bool RuntimeConfig::setSpeedHigh(uint16_t value) {
    if (value < 20 || value > 90) {
        DEBUG_PRINTLN("RuntimeConfig: High speed out of range or invalid order");
        return false;
    }

    _settings.speedHigh = value;
    _isDirty = true;
    notifyCallbacks("speedHigh");
    DEBUG_PRINT("RuntimeConfig: High speed updated to ");
    DEBUG_PRINTLN(value);
    return true;
}

bool RuntimeConfig::setDoorTimeoutMs(unsigned long value) {
    if (value < 1000 || value > 30000) {  // 1-30 seconds
        DEBUG_PRINTLN("RuntimeConfig: Door timeout out of range (1000-30000ms)");
        return false;
    }

    _settings.doorTimeoutMs = value / 1000;  // Store in seconds
    _isDirty = true;
    notifyCallbacks("doorTimeout");
    DEBUG_PRINT("RuntimeConfig: Door timeout updated to ");
    DEBUG_PRINT(value);
    DEBUG_PRINTLN(" ms");
    return true;
}

bool RuntimeConfig::setBuzzerVolume(uint8_t value) {
    if (value > 10) {
        DEBUG_PRINTLN("RuntimeConfig: Buzzer volume out of range (0-10)");
        return false;
    }

    _settings.buzzerVolume = value;
    _isDirty = true;
    notifyCallbacks("buzzerVolume");
    DEBUG_PRINT("RuntimeConfig: Buzzer volume updated to ");
    DEBUG_PRINTLN(value);
    return true;
}

// Flag operations
void RuntimeConfig::setFlag(uint8_t flag, bool value) {
    if (value) {
        _settings.flags |= flag;
    } else {
        _settings.flags &= ~flag;
    }
    _isDirty = true;
}

void RuntimeConfig::setAudioFeedbackEnabled(bool enabled) {
    setFlag(FLAG_AUDIO_FEEDBACK, enabled);
    notifyCallbacks("audioFeedback");
}

void RuntimeConfig::setVoiceRecognitionEnabled(bool enabled) {
    setFlag(FLAG_VOICE_RECOGNITION, enabled);
    notifyCallbacks("voiceRecognition");
}

void RuntimeConfig::setWatchdogEnabled(bool enabled) {
    setFlag(FLAG_WATCHDOG_ENABLED, enabled);
    notifyCallbacks("watchdog");
}

// Callback system
void RuntimeConfig::registerCallback(ConfigChangeCallback callback) {
    if (_callbackCount < MAX_CALLBACKS && callback != nullptr) {
        _callbacks[_callbackCount++] = callback;
    }
}

void RuntimeConfig::notifyCallbacks(const char* key) {
    for (uint8_t i = 0; i < _callbackCount; i++) {
        if (_callbacks[i]) {
            _callbacks[i](key);
        }
    }
}

// ========== SWING PHYSICS SETTER IMPLEMENTATIONS ==========

bool RuntimeConfig::setSwingPeriodMs(uint16_t value) {
    if (value < 1000 || value > 12000) {  // 3-12 seconds reasonable range
        DEBUG_PRINTLN("RuntimeConfig: Swing period out of range (3000-12000ms)");
        return false;
    }
    _settings.swingPeriodMs = value;
    _isDirty = true;
    notifyCallbacks("swingPeriod");
    DEBUG_PRINT("RuntimeConfig: Swing period set to ");
    DEBUG_PRINT(value);
    DEBUG_PRINTLN("ms");
    return true;
}

bool RuntimeConfig::setSwingStepIntervalMs(uint16_t value) {
    if (value < 3 || value > 100) {  // 5-100ms for smooth motion
        DEBUG_PRINTLN("RuntimeConfig: Step interval out of range (5-100ms)");
        return false;
    }
    _settings.swingStepIntervalMs = value;
    _isDirty = true;
    notifyCallbacks("swingStepInterval");
    DEBUG_PRINT("RuntimeConfig: Step interval set to ");
    DEBUG_PRINT(value);
    DEBUG_PRINTLN("ms");
    return true;
}

bool RuntimeConfig::setSwingMaxAngleDegrees(uint8_t value) {
    if (value < 30 || value > 180) {  // 15-90 degrees safe range
        DEBUG_PRINTLN("RuntimeConfig: Max angle out of range (15-90 degrees)");
        return false;
    }
    _settings.swingMaxAngleDegrees = value;
    _isDirty = true;
    notifyCallbacks("swingMaxAngle");
    DEBUG_PRINT("RuntimeConfig: Max swing angle set to ");
    DEBUG_PRINT(value);
    DEBUG_PRINTLN(" degrees");
    return true;
}

bool RuntimeConfig::setSwingSpeedLowSteps(uint8_t value) {
    if (value < 1 || value > 255) {  // Reasonable step range
        DEBUG_PRINTLN("RuntimeConfig: Low speed steps out of range (10-200)");
        return false;
    }
    _settings.swingSpeedLowSteps = value;
    _isDirty = true;
    notifyCallbacks("swingSpeedLow");
    DEBUG_PRINT("RuntimeConfig: Low speed steps set to ");
    DEBUG_PRINTLN(value);
    return true;
}

bool RuntimeConfig::setSwingSpeedMediumSteps(uint8_t value) {
    if (value < 1 || value > 255) {
        DEBUG_PRINTLN("RuntimeConfig: Medium speed steps out of range (20-300)");
        return false;
    }
    _settings.swingSpeedMediumSteps = value;
    _isDirty = true;
    notifyCallbacks("swingSpeedMedium");
    DEBUG_PRINT("RuntimeConfig: Medium speed steps set to ");
    DEBUG_PRINTLN(value);
    return true;
}

bool RuntimeConfig::setSwingSpeedHighSteps(uint8_t value) {
    if (value < 1 || value > 300) {
        DEBUG_PRINTLN("RuntimeConfig: High speed steps out of range (30-400)");
        return false;
    }
    _settings.swingSpeedHighSteps = value;
    _isDirty = true;
    notifyCallbacks("swingSpeedHigh");
    DEBUG_PRINT("RuntimeConfig: High speed steps set to ");
    DEBUG_PRINTLN(value);
    return true;
}

bool RuntimeConfig::setSwingSmoothStopMs(uint16_t value) {
    if (value < 1000 || value > 10000) {  // 1-10 seconds for smooth stopping
        DEBUG_PRINTLN("RuntimeConfig: Smooth stop time out of range (1000-10000ms)");
        return false;
    }
    _settings.swingSmoothStopMs = value;
    _isDirty = true;
    notifyCallbacks("swingSmoothStop");
    DEBUG_PRINT("RuntimeConfig: Smooth stop time set to ");
    DEBUG_PRINT(value);
    DEBUG_PRINTLN("ms");
    return true;
}

// ========== PENDULUM PHYSICS PARAMETERS ==========

bool RuntimeConfig::setPushDurationPercent(uint8_t value) {
    if (value < 5 || value > 50) {  // 5-50% of cycle is reasonable
        DEBUG_PRINTLN("RuntimeConfig: Push duration out of range (5-50%)");
        return false;
    }
    _settings.pushDurationPercent = value;
    _isDirty = true;
    notifyCallbacks("pushDuration");
    DEBUG_PRINT("RuntimeConfig: Push duration set to ");
    DEBUG_PRINT(value);
    DEBUG_PRINTLN("%");
    return true;
}

bool RuntimeConfig::setPushPowerPercent(uint8_t value) {
    if (value < 50 || value > 100) {  // 20-100% power range
        DEBUG_PRINTLN("RuntimeConfig: Push power out of range (20-100%)");
        return false;
    }
    _settings.pushPowerPercent = value;
    _isDirty = true;
    notifyCallbacks("pushPower");
    DEBUG_PRINT("RuntimeConfig: Push power set to ");
    DEBUG_PRINT(value);
    DEBUG_PRINTLN("%");
    return true;
}



void RuntimeConfig::loadDefaultPreset() {
    DEBUG_PRINTLN("RuntimeConfig: Loading default preset");
    loadDefaults(); // Use existing defaults
    save();
}


void RuntimeConfig::factoryReset() {
    DEBUG_PRINTLN("RuntimeConfig: Factory reset");
    loadDefaults();
    saveToEEPROM();
}
