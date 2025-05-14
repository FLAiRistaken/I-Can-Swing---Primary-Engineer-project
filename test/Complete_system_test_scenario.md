# Complete System Test Scenarios for Wheelchair Swing

These test scenarios verify the entire wheelchair swing operation sequence, focusing on key safety features and user interactions. Each scenario includes detailed steps, expected outcomes, and verification points.

## Scenario 1: Normal Operation Sequence

### Test 1A: System Startup and Initialisation
1. **Steps:**
   - Power on the system
   - Observe initialisation sequence
2. **Expected Results:**
   - All components initialize without errors
   - System enters IDLE state
   - Display shows "Swing Ready"
   - Confirmation beeps play through buzzer[5]
3. **Verification:**
   - Check serial output for successful initialisation messages
   - Verify display shows correct status
   - Confirm buzzer produces startup tones

### Test 1B: User Detection and Swing Activation
1. **Steps:**
   - Simulate user sitting on swing (activate pressure sensor)
   - Issue START command (via button or voice)
2. **Expected Results:**
   - System detects user presence
   - Motors start at SPEED_LOW
   - System transitions to SWINGING state[2]
   - Display updates to show SWINGING status
3. **Verification:**
   - Verify `_isUserPresent` flag is set[2]
   - Check state transition to STATE_SWINGING
   - Confirm motors are running (stepperLeft and stepperRight)

### Test 1C: Speed Control
1. **Steps:**
   - With swing in motion, issue SPEED_UP command
   - Observe speed change
   - Issue SPEED_UP again to maximum speed
   - Issue SPEED_DOWN to reduce speed
2. **Expected Results:**
   - Speed transitions: LOW → MEDIUM → HIGH and back
   - Motors adjust to corresponding RPM values (300 → 500 → 700)[6]
   - Display updates to show current speed
3. **Verification:**
   - Monitor state machine's `_currentSpeed` value
   - Check motor speed settings via `getSpeed()`
   - Verify smooth motion changes without jerking

### Test 1D: Normal Shutdown
1. **Steps:**
   - Issue STOP command while swing is in motion
   - Observe stopping behavior
2. **Expected Results:**
   - Motors stop gradually
   - System returns to IDLE state
   - Display updates to IDLE status
3. **Verification:**
   - Verify motors have stopped completely
   - Check state machine returns to STATE_IDLE
   - Confirm motors are disabled after stopping[5]

## Scenario 2: Safety Features

### Test 2A: User Absence Detection
1. **Steps:**
   - Start swing with user present
   - Remove pressure from sensor (simulate user leaving)
2. **Expected Results:**
   - Safety monitor detects user absence[3]
   - Swing stops immediately
   - System returns to IDLE state[2]
   - Buzzer issues alert
3. **Verification:**
   - Verify `SafetyMonitor::checkUserPresence()` returns STATUS_EMERGENCY[4]
   - Check state transition to STATE_IDLE
   - Confirm motors have been disabled

### Test 2B: Obstacle Detection
1. **Steps:**
   - Start swing in normal operation
   - Place obstacle within ultrasonic sensor range (25cm)
2. **Expected Results:**
   - Safety monitor detects obstacle[3]
   - System transitions to ERROR state
   - Motors stop immediately
   - Buzzer issues alert
3. **Verification:**
   - Verify `_frontDistance` or `_rearDistance` is within warning threshold
   - Check state transition to STATE_ERROR
   - Confirm `handleSafetyStatus()` triggered appropriate event[3]

### Test 2C: Ground Detection Filtering
1. **Steps:**
   - Start swing in normal operation
   - Observe swing at its lowest point near ground
2. **Expected Results:**
   - Ultrasonic sensors may detect ground
   - System filters out ground readings correctly[3]
   - Swing continues normal operation without false emergency
3. **Verification:**
   - Check `isReadingExpectedGround()` correctly identifies ground pattern
   - Verify swing phase is being tracked accurately
   - Confirm no false obstacle detections

### Test 2D: Motor Stall Detection
1. **Steps:**
   - Simulate motor stall condition (set speed to 0 while active)
   - Call `updateMotorStatus(true, 0)` three times
2. **Expected Results:**
   - Safety monitor detects motor stall after multiple occurrences[3]
   - System transitions to EMERGENCY state
   - Display shows emergency status
3. **Verification:**
   - Check `_stallCount` increments appropriately
   - Verify state transition to STATE_EMERGENCY
   - Confirm motors have been stopped

## Scenario 3: Emergency Handling

### Test 3A: Emergency Stop Button
1. **Steps:**
   - Start swing in normal operation
   - Activate emergency stop button/command
2. **Expected Results:**
   - System immediately transitions to EMERGENCY state
   - All motors stop
   - Buzzer produces emergency tone sequence[2]
   - Display shows emergency status
3. **Verification:**
   - Verify state transition to STATE_EMERGENCY
   - Check that normal commands are ignored during emergency
   - Confirm motors have been disabled

### Test 3B: Emergency Reset Procedure
1. **Steps:**
   - With system in EMERGENCY state
   - Reset emergency button/condition
   - Hold STOP button for 3+ seconds
2. **Expected Results:**
   - System processes EVENT_EMERGENCY_RESET[2]
   - System transitions to IDLE state
   - Buzzer confirms reset completion
3. **Verification:**
   - Check state transition from STATE_EMERGENCY to STATE_IDLE
   - Verify system responds to normal commands again
   - Confirm buzzer produces reset confirmation tones

### Test 3C: Critical Proximity Emergency
1. **Steps:**
   - Start swing in normal operation
   - Place obstacle within critical range (5000ms)[6]
2. **Expected Results:**
   - System detects door operation timeout
   - Actuator stops
   - System transitions to ERROR state
3. **Verification:**
   - Verify door timeout detection triggers correctly
   - Check state transition to STATE_ERROR
   - Confirm actuator is stopped safely

### Test 4C: Door Obstacle Detection
1. **Steps:**
   - Start door opening
   - Place obstacle in door path (within ultrasonic range)
2. **Expected Results:**
   - Safety monitor detects obstacle
   - System transitions to ERROR state
   - Door actuator stops
3. **Verification:**
   - Check `handleSafetyStatus()` processes obstacle event correctly
   - Verify state transition to STATE_ERROR
   - Confirm actuator has stopped

## Scenario 5: Voice Command Integration

### Test 5A: Voice Control Operation
1. **Steps:**
   - With user present, issue voice command "GO"
   - Issue "FASTER" voice command twice
   - Issue "SLOWER" voice command
   - Issue "STOP" voice command
2. **Expected Results:**
   - Commands are recognized and processed correctly
   - System responds with appropriate state and speed changes
   - Motors respond to all commands
3. **Verification:**
   - Check voice recognition correctly maps commands to events
   - Verify state machine transitions correctly
   - Confirm motors respond to all speed changes

### Test 5B: Voice Door Control
1. **Steps:**
   - Issue "DOOR" or "OPEN" voice command
   - Wait for door to fully open
   - Issue "DOOR" or "CLOSE" voice command
2. **Expected Results:**
   - Door actuator responds correctly to voice commands
   - System transitions through appropriate door states
3. **Verification:**
   - Check voice commands properly trigger door toggle events
   - Verify door cycles through states correctly
   - Confirm buzzer provides appropriate feedback