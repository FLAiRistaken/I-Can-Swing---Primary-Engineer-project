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
        float frontLeftWarningDistance;    // 4 bytes
        float frontLeftCriticalDistance;   // 4 bytes
        float frontRightWarningDistance;     // 4 bytes
        float frontRightCriticalDistance;    // 4 bytes

        // Motor Configuration (8 bytes)
        uint16_t speedLow;            // 2 bytes
        uint16_t speedMedium;         // 2 bytes
        uint16_t speedHigh;           // 2 bytes
        uint16_t maxSpeed;            // 2 bytes

        // ========== SWING MOTION PARAMETERS ========== (10 bytes)
        uint16_t swingPeriodMs;           // Total swing cycle time (3000-6000ms) - 2 bytes
        uint16_t swingStepIntervalMs;     // Time between micro-steps (10-50ms) - 2 bytes
        uint8_t swingMaxAngleDegrees;     // Maximum swing angle (30-60°) - 1 byte
        uint8_t swingSpeedLowSteps;       // Steps per interval at low speed - 1 byte
        uint8_t swingSpeedMediumSteps;    // Steps per interval at medium speed - 1 byte
        uint8_t swingSpeedHighSteps;      // Steps per interval at high speed - 1 byte
        uint16_t swingSmoothStopMs;       // Time for smooth stop (1000-5000ms) - 2 bytes
        uint8_t pushDurationPercent;     // Push phase duration (10-50%) - 1 byte
        uint8_t pushPowerPercent;        // Push power level (50-100%) - 1 byte

        // System Configuration (8 bytes)
        uint16_t pressureThreshold;   // 2 bytes
        uint16_t doorTimeoutMs;       // 2 bytes (stored in seconds, multiply by 1000)
        uint8_t buzzerVolume;         // 1 byte (0-10)
        uint8_t flags;                // 1 byte - bit flags for boolean settings
        uint16_t safetyCheckInterval; // 2 bytes

    };

    // Flag bit definitions
    static const uint8_t FLAG_AUDIO_FEEDBACK = 0x01;
    static const uint8_t FLAG_VOICE_RECOGNITION = 0x02;
    static const uint8_t FLAG_WATCHDOG_ENABLED = 0x04;

    // Simple callback type (no std::function)
    typedef void (*ConfigChangeCallback)(const char* key);

    static RuntimeConfig& getInstance();

    void begin();
    void save();
    void factoryReset();
    bool isValid() const;

    // Simple getters
    float getFrontLeftWarningDistance() const { return _settings.frontLeftWarningDistance; }
    float getFrontLeftCriticalDistance() const { return _settings.frontLeftCriticalDistance; }
    float getFrontRightWarningDistance() const { return _settings.frontRightWarningDistance; }
    float getFrontRightCriticalDistance() const { return _settings.frontRightCriticalDistance; }
    uint16_t getPressureThreshold() const { return _settings.pressureThreshold; }

    uint16_t getSpeedLow() const { return _settings.speedLow; }
    uint16_t getSpeedMedium() const { return _settings.speedMedium; }
    uint16_t getSpeedHigh() const { return _settings.speedHigh; }
    uint16_t getMaxSpeed() const { return _settings.maxSpeed; }

    unsigned long getDoorTimeoutMs() const { return _settings.doorTimeoutMs * 1000UL; }
    uint8_t getBuzzerVolume() const { return _settings.buzzerVolume; }
    uint16_t getSafetyCheckInterval() const { return _settings.safetyCheckInterval; }

    // Swing motion getters
    uint16_t getSwingPeriodMs() const { return _settings.swingPeriodMs; }
    uint16_t getSwingStepIntervalMs() const { return _settings.swingStepIntervalMs; }
    uint8_t getSwingMaxAngleDegrees() const { return _settings.swingMaxAngleDegrees; }
    uint8_t getSwingSpeedLowSteps() const { return _settings.swingSpeedLowSteps; }
    uint8_t getSwingSpeedMediumSteps() const { return _settings.swingSpeedMediumSteps; }
    uint8_t getSwingSpeedHighSteps() const { return _settings.swingSpeedHighSteps; }
    uint16_t getSwingSmoothStopMs() const { return _settings.swingSmoothStopMs; }
    uint8_t getPushDurationPercent() const { return _settings.pushDurationPercent; }
    uint8_t getPushPowerPercent() const { return _settings.pushPowerPercent; }

    // Swing mottion setters
    bool setSwingPeriodMs(uint16_t value);
    bool setSwingStepIntervalMs(uint16_t value);
    bool setSwingMaxAngleDegrees(uint8_t value);
    bool setSwingSpeedLowSteps(uint8_t value);
    bool setSwingSpeedMediumSteps(uint8_t value);
    bool setSwingSpeedHighSteps(uint8_t value);
    bool setSwingSmoothStopMs(uint16_t value);

    // For advanced pendulum physics control
    bool setPushDurationPercent(uint8_t value);    // Control push phase duration
    bool setPushPowerPercent(uint8_t value);       // Control push power (0-100%)


    // Flag getters
    bool isAudioFeedbackEnabled() const { return _settings.flags & FLAG_AUDIO_FEEDBACK; }
    bool isVoiceRecognitionEnabled() const { return _settings.flags & FLAG_VOICE_RECOGNITION; }
    bool isWatchdogEnabled() const { return _settings.flags & FLAG_WATCHDOG_ENABLED; }

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

    // Simple callback system (max 3 callbacks)
    void registerCallback(ConfigChangeCallback callback);


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
