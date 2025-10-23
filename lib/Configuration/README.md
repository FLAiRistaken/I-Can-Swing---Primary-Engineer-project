# Runtime Configuration Module

This directory contains the `RuntimeConfig` module, a critical component that manages all the system's dynamic, user-adjustable settings. It allows key parameters to be changed "at runtime" (e.g., via the web interface) and persists them across power cycles.

## 1. Overview

The purpose of `RuntimeConfig` is to separate tunable parameters (like swing speed and sensor sensitivity) from static hardware constants (like pin numbers, which are in `include/Configuration.h`). This creates a flexible system that can be easily tuned without recompiling firmware.

It is implemented as a **Singleton**, meaning there is only one instance of `RuntimeConfig` for the entire system, accessible via `RuntimeConfig::getInstance()`.

## 2. Key Features

*   **Persistent Storage:** All settings are saved to the Arduino's built-in **EEPROM**. This ensures that your finely-tuned parameters are not lost when the device is powered off and on again.
*   **Data Integrity:** The system uses a multi-layered approach to ensure the configuration data loaded from EEPROM is valid:
    1.  A **Magic Number** confirms the EEPROM has been initialised by this application.
    2.  A **Version Number** allows for safe future upgrades to the settings structure.
    3.  A **Checksum** verifies that the data has not been corrupted.
    If any of these checks fail, the system reverts to a safe, default configuration.
*   **Live Updates via Callbacks:** Other modules can "subscribe" to configuration changes. When a setting is updated (e.g., via the web UI), a callback is triggered, allowing other parts of the system to react to the change instantly without needing to constantly poll for new values.
*   **Configuration Presets:** The module includes pre-defined presets (`Safe`, `Default`, `Testing`) that allow for quickly switching between different sets of operational parameters. A `factoryReset()` function is also available to restore the original default settings.

## 3. The `Settings` Structure

All configurable parameters are stored in a single `struct` named `Settings`. This includes:
*   **Safety Parameters:** `frontWarningDistance`, `frontCriticalDistance`, `pressureThreshold`.
*   **Swing Motion Parameters:** All the values needed for the physics-based swing algorithm, such as `swingPeriodMs`, `swingMaxAngleDegrees`, and `swingSpeed...Steps`.
*   **System Parameters:** `doorTimeoutMs`, `buzzerVolume`.
*   **Feature Flags:** A single byte used to enable or disable major features like audio feedback and voice recognition.

This `struct` is what gets written to and read from the EEPROM.

## 4. How It Works

1.  On startup, `RuntimeConfig::begin()` is called.
2.  It attempts to load the `Settings` struct from EEPROM.
3.  It validates the loaded data using the magic number, version, and checksum.
4.  If the data is valid, the system uses the saved settings.
5.  If the data is invalid (or it's the first ever boot), it calls `loadDefaults()` to populate the settings with safe, known-good values, and then saves these to the EEPROM for the next boot.
6.  When a module like the `WebServer` calls a setter function (e.g., `setBuzzerVolume()`), the new value is validated and stored. A "dirty flag" is set.
7.  The `notifyCallbacks()` function is then called to instantly inform any subscribed modules of the change.
8.  Periodically, or when triggered, the `save()` method writes the settings to the EEPROM, but only if the dirty flag is set, preventing unnecessary writes.

## 5. Dependencies

`RuntimeConfig` is a central service module used by many other parts of the system:
*   **`StepperDriver`**: Reads all swing motion parameters to control its physics algorithm.
*   **`SafetyMonitor`**: Reads safety thresholds for obstacle and pressure detection.
*   **`DoorActuatorManager`**: Reads the door timeout value.
*   **`WebServer`**: Provides the user interface for modifying these settings at runtime.
