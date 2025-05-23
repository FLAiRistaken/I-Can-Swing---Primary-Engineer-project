// lib/Configuration/RuntimeConfig.h
#pragma once

#include <Arduino.h>
#include <EEPROM.h>

class RuntimeConfig {
public:
    // Compact configuration structure - fits in ~100 bytes
    struct Settings {
        uint16_t magic;           // Magic number for validation (2 bytes)
        uint8_t version;          // Configuration version (1 byte)
        uint8_t checksum;         // Simple checksum (1 byte)

        // Safety Configuration (16 bytes)
        float frontWarningDistance;    // 4 bytes
        float frontCriticalDistance;   // 4 bytes
        float rearWarningDistance;     // 4 bytes
        float rearCriticalDistance;    // 4 bytes

        // Motor Configuration (8 bytes)
        uint16_t speedLow;            // 2 bytes
        uint16_t speedMedium;         // 2 bytes
        uint16_t speedHigh;           // 2 bytes
        uint16_t maxSpeed;            // 2 bytes

        // System Configuration (8 bytes)
        uint16_t pressureThreshold;   // 2 bytes
        uint16_t doorTimeoutMs;       // 2 bytes (stored in seconds, multiply by 1000)
        uint8_t buzzerVolume;         // 1 byte (0-10)
        uint8_t flags;                // 1 byte - bit flags for boolean settings
        uint16_t safetyCheckInterval; // 2 bytes

        // Calibration Data (16 bytes)
        float frontSensorBaseline;    // 4 bytes
        float rearSensorBaseline;     // 4 bytes
        float pressureBaseline;       // 4 bytes
        uint32_t lastCalibrationTime; // 4 bytes

        // Usage Statistics (8 bytes)
        uint16_t totalSwingCycles;    // 2 bytes
        uint16_t totalDoorOperations; // 2 bytes
        uint16_t totalEmergencyStops; // 2 bytes
        uint16_t operationHours;      // 2 bytes

        // Reserved for future use (8 bytes)
        uint32_t reserved1;           // 4 bytes
        uint32_t reserved2;           // 4 bytes
    }; // Total: ~64 bytes

    // Flag bit definitions
    static const uint8_t FLAG_AUDIO_FEEDBACK = 0x01;
    static const uint8_t FLAG_VOICE_RECOGNITION = 0x02;
    static const uint8_t FLAG_WATCHDOG_ENABLED = 0x04;
    static const uint8_t FLAG_CALIBRATION_VALID = 0x08;
    static const uint8_t FLAG_DEMO_MODE = 0x10;
    static const uint8_t FLAG_SAFETY_OVERRIDE = 0x20;

    // Simple callback type (no std::function)
    typedef void (*ConfigChangeCallback)(const char* key);

    static RuntimeConfig& getInstance();

    void begin();
    void save();
    void factoryReset();
    bool isValid() const;

    // Simple getters
    float getFrontWarningDistance() const { return _settings.frontWarningDistance; }
    float getFrontCriticalDistance() const { return _settings.frontCriticalDistance; }
    float getRearWarningDistance() const { return _settings.rearWarningDistance; }
    float getRearCriticalDistance() const { return _settings.rearCriticalDistance; }
    uint16_t getPressureThreshold() const { return _settings.pressureThreshold; }

    uint16_t getSpeedLow() const { return _settings.speedLow; }
    uint16_t getSpeedMedium() const { return _settings.speedMedium; }
    uint16_t getSpeedHigh() const { return _settings.speedHigh; }
    uint16_t getMaxSpeed() const { return _settings.maxSpeed; }

    unsigned long getDoorTimeoutMs() const { return _settings.doorTimeoutMs * 1000UL; }
    uint8_t getBuzzerVolume() const { return _settings.buzzerVolume; }
    uint16_t getSafetyCheckInterval() const { return _settings.safetyCheckInterval; }

    // Flag getters
    bool isAudioFeedbackEnabled() const { return _settings.flags & FLAG_AUDIO_FEEDBACK; }
    bool isVoiceRecognitionEnabled() const { return _settings.flags & FLAG_VOICE_RECOGNITION; }
    bool isWatchdogEnabled() const { return _settings.flags & FLAG_WATCHDOG_ENABLED; }
    bool isCalibrationValid() const { return _settings.flags & FLAG_CALIBRATION_VALID; }
    bool isDemoModeEnabled() const { return _settings.flags & FLAG_DEMO_MODE; }

    // Simple setters with validation
    bool setWarningDistance(float value);
    bool setCriticalDistance(float value);
    bool setPressureThreshold(uint16_t value);
    bool setSpeedLow(uint16_t value);
    bool setSpeedMedium(uint16_t value);
    bool setSpeedHigh(uint16_t value);
    bool setDoorTimeoutMs(unsigned long value);
    bool setBuzzerVolume(uint8_t value);

    // Flag setters
    void setAudioFeedbackEnabled(bool enabled);
    void setVoiceRecognitionEnabled(bool enabled);
    void setWatchdogEnabled(bool enabled);
    void setCalibrationValid(bool valid);
    void setDemoModeEnabled(bool enabled);

    // Usage tracking
    void incrementSwingCycles();
    void incrementDoorOperations();
    void incrementEmergencyStops();
    void incrementOperationHours();

    // Calibration support
    bool updateCalibrationData(const String& sensor, float baseline);
    float getSensorBaseline(const String& sensor) const;

    // Simple callback system (max 3 callbacks)
    void registerCallback(ConfigChangeCallback callback);

    // JSON export (simplified)
    String exportToJson() const;

    // Configuration presets
    void loadSafePreset();      // Conservative settings
    void loadDefaultPreset();   // Balanced settings
    void loadTestingPreset();   // For development/testing

private:
    RuntimeConfig() : _isDirty(false), _callbackCount(0) {
        // Initialize settings to zero
        memset(&_settings, 0, sizeof(Settings));

        // Initialize callback array to null pointers
        for (uint8_t i = 0; i < MAX_CALLBACKS; i++) {
            _callbacks[i] = nullptr;
        }
    }

    Settings _settings;
    bool _isDirty;

    // Simple callback system
    static const uint8_t MAX_CALLBACKS = 3;
    ConfigChangeCallback _callbacks[MAX_CALLBACKS];
    uint8_t _callbackCount;

    // EEPROM constants
    static const uint16_t EEPROM_ADDRESS = 0;
    static const uint16_t MAGIC_NUMBER = 0xABCD;
    static const uint8_t CONFIG_VERSION = 1;

    // Helper methods
    void loadDefaults();
    bool loadFromEEPROM();
    bool saveToEEPROM();
    uint8_t calculateChecksum() const;
    void notifyCallbacks(const char* key);
    bool validate() const;
    void setFlag(uint8_t flag, bool value);
};
