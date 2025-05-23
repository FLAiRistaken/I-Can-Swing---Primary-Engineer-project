// lib/Configuration/RuntimeConfig.cpp
#include "RuntimeConfig.h"
#include "Configuration.h"

RuntimeConfig& RuntimeConfig::getInstance() {
    static RuntimeConfig instance;
    return instance;
}

void RuntimeConfig::begin() {
    Serial.println("RuntimeConfig: Initializing...");

    EEPROM.begin();
    _isDirty = false;
    _callbackCount = 0;

    if (!loadFromEEPROM() || !isValid()) {
        Serial.println("RuntimeConfig: Loading defaults");
        loadDefaults();
        saveToEEPROM();
    } else {
        Serial.println("RuntimeConfig: Loaded from EEPROM");
    }

    Serial.print("RuntimeConfig: Version ");
    Serial.print(_settings.version);
    Serial.print(", Size: ");
    Serial.print(sizeof(Settings));
    Serial.println(" bytes");
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
    Serial.println("RuntimeConfig: Saved to EEPROM");
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
// REPLACE the existing setWarningDistance method with this:
bool RuntimeConfig::setWarningDistance(float value) {
    if (value < 5.0f || value > 200.0f || value <= _settings.frontCriticalDistance) {
        Serial.println("RuntimeConfig: Warning distance out of range (5-200cm) or <= critical distance");
        return false;
    }

    // Set both front and rear warning distances to same value
    _settings.frontWarningDistance = value;
    _settings.rearWarningDistance = value;
    _isDirty = true;
    notifyCallbacks("warningDistance");
    Serial.print("RuntimeConfig: Warning distance updated to ");
    Serial.println(value);
    return true;
}


bool RuntimeConfig::setCriticalDistance(float value) {
    if (value < 1.0f || value > 50.0f || value >= _settings.frontWarningDistance) {
        Serial.println("RuntimeConfig: Critical distance out of range or >= warning distance");
        return false;
    }

    // Set both front and rear critical distances to same value
    _settings.frontCriticalDistance = value;
    _settings.rearCriticalDistance = value;
    _isDirty = true;
    notifyCallbacks("criticalDistance");
    Serial.print("RuntimeConfig: Critical distance updated to ");
    Serial.println(value);
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
        Serial.println("RuntimeConfig: Medium speed out of range or invalid order");
        return false;
    }

    _settings.speedMedium = value;
    _isDirty = true;
    notifyCallbacks("speedMedium");
    Serial.print("RuntimeConfig: Medium speed updated to ");
    Serial.println(value);
    return true;
}

bool RuntimeConfig::setSpeedHigh(uint16_t value) {
    if (value < 300 || value > 700 || value <= _settings.speedMedium || value > _settings.maxSpeed) {
        Serial.println("RuntimeConfig: High speed out of range or invalid order");
        return false;
    }

    _settings.speedHigh = value;
    _isDirty = true;
    notifyCallbacks("speedHigh");
    Serial.print("RuntimeConfig: High speed updated to ");
    Serial.println(value);
    return true;
}

bool RuntimeConfig::setDoorTimeoutMs(unsigned long value) {
    if (value < 1000 || value > 30000) {  // 1-30 seconds
        Serial.println("RuntimeConfig: Door timeout out of range (1000-30000ms)");
        return false;
    }

    _settings.doorTimeoutMs = value / 1000;  // Store in seconds
    _isDirty = true;
    notifyCallbacks("doorTimeout");
    Serial.print("RuntimeConfig: Door timeout updated to ");
    Serial.print(value);
    Serial.println(" ms");
    return true;
}

bool RuntimeConfig::setBuzzerVolume(uint8_t value) {
    if (value > 10) {
        Serial.println("RuntimeConfig: Buzzer volume out of range (0-10)");
        return false;
    }

    _settings.buzzerVolume = value;
    _isDirty = true;
    notifyCallbacks("buzzerVolume");
    Serial.print("RuntimeConfig: Buzzer volume updated to ");
    Serial.println(value);
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

// Usage tracking
void RuntimeConfig::incrementSwingCycles() {
    if (_settings.totalSwingCycles < 65535) {
        _settings.totalSwingCycles++;
        _isDirty = true;
    }
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

// JSON export (simplified for Arduino)
String RuntimeConfig::exportToJson() const {
    String json = "{";
    json += "\"frontWarning\":" + String(_settings.frontWarningDistance) + ",";
    json += "\"frontCritical\":" + String(_settings.frontCriticalDistance) + ",";
    json += "\"rearWarning\":" + String(_settings.rearWarningDistance) + ",";
    json += "\"rearCritical\":" + String(_settings.rearCriticalDistance) + ",";
    json += "\"pressureThreshold\":" + String(_settings.pressureThreshold) + ",";
    json += "\"speedLow\":" + String(_settings.speedLow) + ",";
    json += "\"speedMedium\":" + String(_settings.speedMedium) + ",";
    json += "\"speedHigh\":" + String(_settings.speedHigh) + ",";
    json += "\"audioEnabled\":" + String(isAudioFeedbackEnabled() ? "true" : "false") + ",";
    json += "\"voiceEnabled\":" + String(isVoiceRecognitionEnabled() ? "true" : "false");
    json += "}";
    return json;
}

void RuntimeConfig::factoryReset() {
    Serial.println("RuntimeConfig: Factory reset");
    loadDefaults();
    saveToEEPROM();
}
