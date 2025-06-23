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

    if (!loadFromEEPROM() || !isValid()) {
        DEBUG_PRINTLN("RuntimeConfig: Loading defaults");
        loadDefaults();
        saveToEEPROM();
    } else {
        DEBUG_PRINTLN("RuntimeConfig: Loaded from EEPROM");
    }

    DEBUG_PRINT("RuntimeConfig: Version ");
    DEBUG_PRINT(_settings.version);
    DEBUG_PRINT(", Size: ");
    DEBUG_PRINT(sizeof(Settings));
    DEBUG_PRINTLN(" bytes");
}

void RuntimeConfig::loadDefaults() {
    memset(&_settings, 0, sizeof(Settings));

    _settings.magic = MAGIC_NUMBER;
    _settings.version = CONFIG_VERSION;

    // Load from your existing Configuration.h
    _settings.frontWarningDistance = OBSTACLE_DISTANCE_CM;
    _settings.frontCriticalDistance = CRITICAL_DISTANCE_CM;
    _settings.rearWarningDistance = OBSTACLE_DISTANCE_CM;
    _settings.rearCriticalDistance = CRITICAL_DISTANCE_CM;
    _settings.pressureThreshold = PRESSURE_THRESHOLD;

    _settings.speedLow = SPEED_LOW;
    _settings.speedMedium = SPEED_MEDIUM;
    _settings.speedHigh = SPEED_HIGH;
    _settings.maxSpeed = 700;

    _settings.doorTimeoutMs = DOOR_OPEN_TIME_MS / 1000; // Store in seconds
    _settings.buzzerVolume = 5;
    _settings.safetyCheckInterval = SENSOR_CHECK_MS;

    // Set default flags
    _settings.flags = FLAG_AUDIO_FEEDBACK | FLAG_VOICE_RECOGNITION | FLAG_WATCHDOG_ENABLED;

    // Calibration defaults
    _settings.frontSensorBaseline = 200.0f;
    _settings.rearSensorBaseline = 200.0f;
    _settings.pressureBaseline = 100.0f;
    _settings.lastCalibrationTime = 0;

    // Usage stats start at zero
    _settings.totalSwingCycles = 0;
    _settings.totalDoorOperations = 0;
    _settings.totalEmergencyStops = 0;
    _settings.operationHours = 0;

    _settings.checksum = calculateChecksum();
    _isDirty = true;
}

bool RuntimeConfig::loadFromEEPROM() {
    EEPROM.get(EEPROM_ADDRESS, _settings);
    return (_settings.magic == MAGIC_NUMBER && _settings.version == CONFIG_VERSION);
}

bool RuntimeConfig::saveToEEPROM() {
    if (!_isDirty) return true;

    _settings.checksum = calculateChecksum();
    EEPROM.put(EEPROM_ADDRESS, _settings);
    _isDirty = false;
    DEBUG_PRINTLN("RuntimeConfig: Saved to EEPROM");
    return true;
}

void RuntimeConfig::save() {
    saveToEEPROM();
}

bool RuntimeConfig::isValid() const {
    return (_settings.checksum == calculateChecksum()) && validate();
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
    // Basic validation
    return (_settings.frontWarningDistance > _settings.frontCriticalDistance &&
            _settings.rearWarningDistance > _settings.rearCriticalDistance &&
            _settings.speedLow < _settings.speedMedium &&
            _settings.speedMedium < _settings.speedHigh &&
            _settings.pressureThreshold > 100 &&
            _settings.pressureThreshold < 900);
}

// Simple setters with validation
bool RuntimeConfig::setWarningDistance(float value) {
    if (value < 5.0f || value > 200.0f || value <= _settings.frontCriticalDistance) {
        DEBUG_PRINTLN("RuntimeConfig: Warning distance out of range (5-200cm) or <= critical distance");
        return false;
    }

    // Set both front and rear warning distances to same value
    _settings.frontWarningDistance = value;
    _settings.rearWarningDistance = value;
    _isDirty = true;
    notifyCallbacks("warningDistance");
    DEBUG_PRINT("RuntimeConfig: Warning distance updated to ");
    DEBUG_PRINTLN(value);
    return true;
}


bool RuntimeConfig::setCriticalDistance(float value) {
    if (value < 1.0f || value > 50.0f || value >= _settings.frontWarningDistance) {
        DEBUG_PRINTLN("RuntimeConfig: Critical distance out of range or >= warning distance");
        return false;
    }

    // Set both front and rear critical distances to same value
    _settings.frontCriticalDistance = value;
    _settings.rearCriticalDistance = value;
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
    if (value < 100 || value > 500 || value >= _settings.speedMedium) {
        return false;
    }

    _settings.speedLow = value;
    _isDirty = true;
    notifyCallbacks("speedLow");
    return true;
}

bool RuntimeConfig::setSpeedMedium(uint16_t value) {
    if (value < 200 || value > 600 || value <= _settings.speedLow || value >= _settings.speedHigh) {
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
    if (value < 300 || value > 700 || value <= _settings.speedMedium || value > _settings.maxSpeed) {
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

// Add these demo implementation methods at the end of RuntimeConfig.cpp:

void RuntimeConfig::loadSafePreset() {
    DEBUG_PRINTLN("RuntimeConfig: Loading safe preset");
    setWarningDistance(50.0f);
    setCriticalDistance(15.0f);
    setSpeedLow(200);
    setSpeedMedium(350);
    setSpeedHigh(500);
    setBuzzerVolume(8);
    setAudioFeedbackEnabled(true);
    save();
}

void RuntimeConfig::loadDefaultPreset() {
    DEBUG_PRINTLN("RuntimeConfig: Loading default preset");
    loadDefaults(); // Use existing defaults
    save();
}

void RuntimeConfig::loadTestingPreset() {
    DEBUG_PRINTLN("RuntimeConfig: Loading testing preset");
    setWarningDistance(100.0f);
    setCriticalDistance(20.0f);
    setSpeedLow(150);
    setSpeedMedium(300);
    setSpeedHigh(450);
    save();
}

void RuntimeConfig::factoryReset() {
    DEBUG_PRINTLN("RuntimeConfig: Factory reset");
    loadDefaults();
    saveToEEPROM();
}
