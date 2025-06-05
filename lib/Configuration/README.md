# RuntimeConfig

## **Overview**

RuntimeConfig is a comprehensive, memory-optimised configuration management system for the Arduino-based wheelchair swing control system. It provides persistent storage, real-time configuration changes, demo modes, and usage tracking while maintaining Arduino UNO R4 WiFi compatibility.

---

## **Core Architecture**

### **Design Principles**
- **Memory Optimised**: 64-byte compact structure fits in Arduino EEPROM
- **Type Safe**: Strongly typed getters/setters with validation
- **Persistent**: EEPROM storage with checksum integrity
- **Real-time**: Immediate configuration changes with callback notifications
- **Safety First**: Input validation prevents dangerous configurations

### **Memory Layout**
```cpp
struct Settings {
    // Header (4 bytes)
    uint16_t magic;           // Validation marker (0xABCD)
    uint8_t version;          // Configuration version (1)
    uint8_t checksum;         // Data integrity check

    // Safety Configuration (16 bytes)
    float frontWarningDistance;    // Front sensor warning threshold
    float frontCriticalDistance;   // Front sensor critical threshold
    float rearWarningDistance;     // Rear sensor warning threshold
    float rearCriticalDistance;    // Rear sensor critical threshold

    // Motor Configuration (8 bytes)
    uint16_t speedLow;        // Low speed setting (RPM)
    uint16_t speedMedium;     // Medium speed setting (RPM)
    uint16_t speedHigh;       // High speed setting (RPM)
    uint16_t maxSpeed;        // Maximum allowed speed (RPM)

    // System Configuration (8 bytes)
    uint16_t pressureThreshold;     // Pressure sensor threshold
    uint16_t doorTimeoutMs;         // Door timeout (stored in seconds)
    uint8_t buzzerVolume;           // Buzzer volume (0-10)
    uint8_t flags;                  // Boolean settings as bit flags
    uint16_t safetyCheckInterval;   // Safety check frequency (ms)

    // Calibration Data (16 bytes)
    float frontSensorBaseline;      // Front sensor calibration baseline
    float rearSensorBaseline;       // Rear sensor calibration baseline
    float pressureBaseline;         // Pressure sensor calibration baseline
    uint32_t lastCalibrationTime;  // Last calibration timestamp

    // Usage Statistics (8 bytes)
    uint16_t totalSwingCycles;      // Total swing operations
    uint16_t totalDoorOperations;   // Total door operations
    uint16_t totalEmergencyStops;   // Total emergency stops
    uint16_t operationHours;        // Total operation time

    // Reserved (8 bytes) - Future expansion
    uint32_t reserved1;
    uint32_t reserved2;
}; // Total: 64 bytes
```

---

## **Configuration Categories**

### **1. Safety Configuration**
Controls all safety-related thresholds and behaviors:

| Setting | Type | Range | Description |
|---------|------|-------|-------------|
| `frontWarningDistance` | float | 5.0-200.0 cm | Distance at which front obstacle warnings trigger |
| `frontCriticalDistance` | float | 1.0-50.0 cm | Distance at which front emergency stop triggers |
| `rearWarningDistance` | float | 5.0-200.0 cm | Distance at which rear obstacle warnings trigger |
| `rearCriticalDistance` | float | 1.0-50.0 cm | Distance at which rear emergency stop triggers |

**Validation Rules:**
- Warning distance must be greater than critical distance
- All distances must be positive and within sensor range
- Changes immediately propagate to SafetyMonitor

### **2. Motor Configuration**
Controls motor speeds and limits:

| Setting | Type | Range | Description |
|---------|------|-------|-------------|
| `speedLow` | uint16_t | 100-500 RPM | Low speed setting |
| `speedMedium` | uint16_t | 200-600 RPM | Medium speed setting |
| `speedHigh` | uint16_t | 300-700 RPM | High speed setting |
| `maxSpeed` | uint16_t | 400-1000 RPM | Maximum allowed speed |

**Validation Rules:**
- Speed values must be in ascending order: Low getCurrentState() == StateMachine::STATE_SWINGING) {
        warningThreshold *= 0.7f;  // Reduce by 30% during operation
    }

    // Use thresholds for safety decisions
    if (frontDistance processEvent(StateMachine::EVENT_EMERGENCY);
    }
}
```

### **StateMachine Integration**
StateMachine uses RuntimeConfig for operational parameters:

```cpp
// In StateMachine.cpp
void StateMachine::setSpeed(SpeedLevel level) {
    RuntimeConfig& config = RuntimeConfig::getInstance();

    uint16_t targetSpeed;
    switch (level) {
        case SPEED_LOW:    targetSpeed = config.getSpeedLow(); break;
        case SPEED_MEDIUM: targetSpeed = config.getSpeedMedium(); break;
        case SPEED_HIGH:   targetSpeed = config.getSpeedHigh(); break;
    }

    // Apply speed to motors
    setMotorSpeed(targetSpeed);
}
```

### **Demo System Integration**
The demo system automatically manages configuration during demonstrations:

```cpp
// Start demo - automatically backs up current config
config.setDemoMode(RuntimeConfig::DEMO_GENTLE);

// Demo system applies preset configuration:
// - Speed: 300 RPM
// - Warning distance: 50 cm
// - Critical distance: 15 cm
// - Voice control: Enabled
// - Safety systems: Enhanced

// Stop demo - automatically restores original config
config.stopDemo();
```

---

## **Memory Optimisation Features**

### **Compact Data Types**
- **Bit flags** for boolean values (8 settings in 1 byte)
- **16-bit integers** for most numeric values
- **Strategic padding** to maintain alignment

### **EEPROM Efficiency**
- **Single 64-byte write** for entire configuration
- **Checksum validation** for data integrity
- **Magic number** for corruption detection
- **Version tracking** for future upgrades

### **Callback Optimisation**
- **Function pointers** instead of std::function (no heap allocation)
- **Fixed array** of 3 callbacks maximum
- **Direct notification** without complex event systems

---

## **Safety Considerations**

### **Input Validation**
Every setter method includes comprehensive validation:

```cpp
bool RuntimeConfig::setWarningDistance(float value) {
    // Range validation
    if (value  200.0f) return false;

    // Logic validation (warning > critical)
    if (value  critical)

### **Safe Defaults**
Default configuration prioritises safety:
- **Conservative distances** (30cm warning, 10cm critical)
- **Moderate speeds** (300/500/700 RPM)
- **Enhanced safety features** enabled by default
- **Audio feedback** enabled for user awareness

### **Demo Safety**
Demo modes include safety enhancements:
- **Automatic timeouts** prevent indefinite operation
- **Configuration backup/restore** ensures system integrity
- **Enhanced monitoring** during demonstrations
- **Conservative settings** for public demonstrations

---

## **Error Handling**

### **Validation Failures**
```cpp
bool success = config.setSpeedLow(1000);  // Invalid (too high)
if (!success) {
    Serial.println("Speed validation failed");
    // Current configuration unchanged
}
```

### **EEPROM Failures**
```cpp
config.begin();  // Automatically handles EEPROM failures

// If EEPROM corrupted or invalid:
// 1. Loads safe default configuration
// 2. Saves defaults to EEPROM
// 3. Logs recovery action
// 4. Continues normal operation
```

### **Demo System Failures**
```cpp
// Demo system includes automatic recovery:
// 1. Configuration backup before demo starts
// 2. Automatic restore if demo fails
// 3. Timeout protection prevents stuck demos
// 4. Manual stop capability always available
```

---

## **Performance Characteristics**

### **Memory Usage**
- **Flash Memory**: ~8KB for RuntimeConfig code
- **SRAM Usage**: 64 bytes for configuration + ~200 bytes for demo system
- **EEPROM Usage**: 64 bytes for persistent storage

### **Execution Speed**
- **Configuration reads**: ~1-2 microseconds (direct memory access)
- **Configuration writes**: ~50-100 microseconds (validation + notification)
- **EEPROM save**: ~3-5 milliseconds (infrequent operation)
- **Demo mode switch**: ~10-20 milliseconds (including backup/restore)

### **Real-time Performance**
- **Non-blocking operation**: All operations complete in microseconds
- **Immediate propagation**: Configuration changes take effect instantly
- **Callback notifications**: Registered components notified immediately
- **No heap allocation**: All memory statically allocated

---

## **Future Expansion**

### **Reserved Space**
8 bytes of reserved space allows future expansion without breaking compatibility:

```cpp
// Future features could include:
uint32_t reserved1;  // Could become: maintenance intervals, user profiles
uint32_t reserved2;  // Could become: advanced safety parameters, network config
```

### **Version Migration**
Version tracking enables smooth upgrades:

```cpp
// Future versions can detect and migrate older configurations
if (_settings.version == 1) {
    migrateFromVersion1ToVersion2();
}
```

### **Extensible Demo System**
Demo system designed for easy expansion:

```cpp
// Additional demo modes can be added:
DEMO_THERAPY_SESSION = 5,    // Therapeutic movement patterns
DEMO_MAINTENANCE = 6,        // Maintenance and calibration demos
DEMO_TRAINING = 7,          // Caretaker training demonstrations
```