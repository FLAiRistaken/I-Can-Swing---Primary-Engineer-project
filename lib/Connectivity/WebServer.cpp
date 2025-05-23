// lib/Connectivity/WebServer.cpp
#include "WebServer.h"
#include "RuntimeConfig.h"

WebServer::WebServer(StateMachine* stateMachine, SafetyMonitor* safetyMonitor)
    : _server(80), _stateMachine(stateMachine), _safetyMonitor(safetyMonitor),
      _calibrationActive(false), _calibrationStartTime(0), _calibrationStep(0),
      _currentSensorCalibrating(""), _motorTestActive(false), _currentMotorTest(""),
      _motorTestStartTime(0), _motorTestStep(0), _leftMotorPosition(0),
      _rightMotorPosition(0), _motorTestSafetyCheck(true), _safetyTestActive(false),
      _currentSafetyTest(""), _safetyTestStartTime(0), _safetyTestEndTime(0),
      _safetyTestStep(0), _safetyTestThreshold(0.0f), _safetyOverrideEnabled(false),
      _safetyOverrideTimeout(0), _safetyLogIndex(0) {}

void WebServer::begin(int port) {
    _server.begin();
    Serial.print("Web Server started on port ");
    Serial.println(port);
}

void WebServer::handleClient() {
    WiFiClient client = _server.available();

    if (client) {
        Serial.println("New client connected");
        String request = "";

        while (client.connected()) {
            if (client.available()) {
                String line = client.readStringUntil('\r');
                request += line;

                if (line.length() == 1 && line[0] == '\n') {
                    break;
                }
            }
        }

        // Parse request
        if (request.indexOf("GET / ") >= 0) {
            sendHomePage(client);
        } else if (request.indexOf("GET /control") >= 0) {
            sendControlPage(client);
        } else if (request.indexOf("GET /status") >= 0) {
            sendStatusPage(client);
        } else if (request.indexOf("GET /config") >= 0) {
            sendConfigPage(client);
        } else if (request.indexOf("GET /debug") >= 0) {
            sendDebugPage(client);
        } else if (request.indexOf("GET /safety-test") >= 0) {
            sendSafetyTestPage(client);
        } else if (request.indexOf("GET /api/safety-test") >= 0) {
            // Extract safety test command
            int apiStart = request.indexOf("/api/safety-test") + 16;
            int apiEnd = request.indexOf(" ", apiStart);
            String command = request.substring(apiStart, apiEnd);
            handleSafetyTestAPI(client, command);
        } else if (request.indexOf("GET /motor-test") >= 0) {
            sendMotorTestPage(client);
        } else if (request.indexOf("GET /api/motor-test") >= 0) {
            int apiStart = request.indexOf("/api/motor-test") + 15;
            int apiEnd = request.indexOf(" ", apiStart);
            String command = request.substring(apiStart, apiEnd);
            handleMotorTestAPI(client, command);
        } else if (request.indexOf("GET /calibration") >= 0) {
            sendCalibrationPage(client);
        } else if (request.indexOf("GET /api/calibrate") >= 0) {
            // Extract calibration command
            int apiStart = request.indexOf("/api/calibrate") + 14;
            int apiEnd = request.indexOf(" ", apiStart);
            String command = request.substring(apiStart, apiEnd);
            handleCalibrationAPI(client, command);
        } else if (request.indexOf("GET /api/config-update") >= 0) {
            int paramsStart = request.indexOf("?") + 1;
            int paramsEnd = request.indexOf(" HTTP", paramsStart);
            String params = request.substring(paramsStart, paramsEnd);
            handleConfigUpdate(client, params);
        } else if (request.indexOf("GET /api/config-export") >= 0) {
            exportConfiguration(client);
        } else if (request.indexOf("GET /api/") >= 0) {
            int apiStart = request.indexOf("/api/") + 5;
            int apiEnd = request.indexOf(" ", apiStart);
            String command = request.substring(apiStart, apiEnd);
            handleControlCommand(client, command);
        } else {
            send404Page(client);
        }

        client.stop();
        Serial.println("Client disconnected");
    }
}

// Handle safety test API requests
void WebServer::handleSafetyTestAPI(WiFiClient& client, String command) {
    Serial.print("Safety Test API Command: ");
    Serial.println(command);

    // Check if safety override is enabled or if we're trying to disable it
    if (!_safetyOverrideEnabled && !command.startsWith("-override") && !command.startsWith("-status")) {
        sendJsonResponse(client, "{\"error\":\"Safety override must be enabled for testing\"}");
        return;
    }

    // Handle timeout for safety override
    if (_safetyOverrideEnabled) {
        unsigned long currentTime = millis();
        if (currentTime > _safetyOverrideTimeout) {
            _safetyOverrideEnabled = false;
            Serial.println("Safety override automatically disabled due to timeout");
            if (!command.startsWith("-override") && !command.startsWith("-status")) {
                sendJsonResponse(client, "{\"error\":\"Safety override timeout - must be re-enabled\"}");
                return;
            }
        }
    }

    if (command.startsWith("-trigger")) {
        triggerSafetyEvent(client, command);
    } else if (command.startsWith("-threshold")) {
        runThresholdTest(client, command);
    } else if (command.startsWith("-response-time")) {
        measureResponseTime(client, command);
    } else if (command == "-automated-test") {
        runAutomatedTestSequence(client);
    } else if (command == "-status") {
        getSafetyTestStatus(client);
    } else if (command == "-logs") {
        getSafetyEventLogs(client);
    } else if (command.startsWith("-override")) {
        if (command.indexOf("enable") > 0) {
            toggleSafetyOverride(client, true);
        } else if (command.indexOf("disable") > 0) {
            toggleSafetyOverride(client, false);
        } else {
            sendJsonResponse(client, "{\"error\":\"Invalid override command\"}");
        }
    } else if (command == "-reset-logs") {
        resetSafetyLogs(client);
    } else {
        sendJsonResponse(client, "{\"error\":\"Unknown safety test command\"}");
    }
}

// Trigger a specific safety event
void WebServer::triggerSafetyEvent(WiFiClient& client, String params) {
    // Extract parameters: -trigger?event=obstacle&distance=10&sensor=front
    int eventStart = params.indexOf("event=") + 6;
    int eventEnd = params.indexOf("&", eventStart);
    String eventType = params.substring(eventStart, eventEnd);

    bool success = false;
    String description = "";

    _safetyTestActive = true;
    _currentSafetyTest = "trigger_" + eventType;
    _safetyTestStartTime = micros();

    if (eventType == "obstacle") {
        int distanceStart = params.indexOf("distance=") + 9;
        int distanceEnd = params.indexOf("&", distanceStart);
        float distance = params.substring(distanceStart, distanceEnd).toFloat();

        int sensorStart = params.indexOf("sensor=") + 7;
        String sensor = params.substring(sensorStart);

        success = simulateObstacle(distance, sensor);
        description = "Simulated " + sensor + " obstacle at " + String(distance) + "cm";
    } else if (eventType == "user") {
        success = simulateUserDeparture();
        description = "Simulated user departure from swing";
    } else if (eventType == "stall") {
        success = simulateMotorStall();
        description = "Simulated motor stall condition";
    } else {
        sendJsonResponse(client, "{\"error\":\"Unknown safety event type\"}");
        return;
    }

    _safetyTestEndTime = micros();
    unsigned long responseTime = (_safetyTestEndTime - _safetyTestStartTime) / 1000; // Convert to ms

    // Log the event
    logSafetyEvent(eventType, description, success ? "SUCCESS" : "FAILED", responseTime);

    String response = "{\"status\":\"" + String(success ? "success" : "failed") +
                     "\",\"event\":\"" + eventType +
                     "\",\"description\":\"" + description +
                     "\",\"response_time\":" + String(responseTime) + "}";
    sendJsonResponse(client, response);

    Serial.print("Safety event triggered: ");
    Serial.print(eventType);
    Serial.print(" - ");
    Serial.println(success ? "SUCCESS" : "FAILED");
}

// Run threshold testing
void WebServer::runThresholdTest(WiFiClient& client, String params) {
    // Extract parameters: -threshold?sensor=front&start=100&end=5&steps=20
    int sensorStart = params.indexOf("sensor=") + 7;
    int sensorEnd = params.indexOf("&", sensorStart);
    String sensor = params.substring(sensorStart, sensorEnd);

    int startValStart = params.indexOf("start=") + 6;
    int startValEnd = params.indexOf("&", startValStart);
    float startVal = params.substring(startValStart, startValEnd).toFloat();

    int endValStart = params.indexOf("end=") + 4;
    int endValEnd = params.indexOf("&", endValStart);
    float endVal = params.substring(endValStart, endValEnd).toFloat();

    int stepsStart = params.indexOf("steps=") + 6;
    int steps = params.substring(stepsStart).toInt();

    if (steps <= 0 || steps > 100) {
        steps = 20; // Default to 20 steps
    }

    _safetyTestActive = true;
    _currentSafetyTest = "threshold_" + sensor;
    _safetyTestStartTime = millis();
    _safetyTestStep = 0;
    _safetyTestThreshold = startVal;

    String response = "{\"status\":\"started\",\"test\":\"threshold\",\"sensor\":\"" + sensor +
                     "\",\"start\":" + String(startVal) +
                     ",\"end\":" + String(endVal) +
                     ",\"steps\":" + String(steps) + "}";
    sendJsonResponse(client, response);

    // Perform threshold test - adjust gradually from start to end value
    float stepSize = (startVal - endVal) / steps;

    for (int i = 0; i <= steps; i++) {
        float currentThreshold = startVal - (stepSize * i);

        // Set the threshold in RuntimeConfig temporarily for testing
       RuntimeConfig& config = RuntimeConfig::getInstance();
       config.setWarningDistance(currentThreshold);

        // Simulate obstacle at current threshold
        bool detected = simulateObstacle(currentThreshold, sensor);
        String stepDescription = "Threshold test: " + sensor + " at " + String(currentThreshold) + "cm";

        // Log this step
        logSafetyEvent("threshold", stepDescription,
                       detected ? "DETECTED" : "NOT DETECTED", 0);

        // Wait briefly between steps
        delay(500);

        // If a safety event was triggered, end the test
        if (detected && _stateMachine->getCurrentState() != StateMachine::STATE_IDLE) {
            break;
        }
    }

    // Reset thresholds to defaults after test
    RuntimeConfig& config = RuntimeConfig::getInstance();
    config.setWarningDistance(OBSTACLE_DISTANCE_CM);
    config.setCriticalDistance(CRITICAL_DISTANCE_CM);
    config.save(); // Save the reset values


    _safetyTestActive = false;
    _currentSafetyTest = "";

    Serial.println("Threshold test completed");
}

// Measure response time for a safety event
void WebServer::measureResponseTime(WiFiClient& client, String params) {
    // Extract parameters: -response-time?event=obstacle&iterations=5
    int eventStart = params.indexOf("event=") + 6;
    int eventEnd = params.indexOf("&", eventStart);
    String eventType = params.substring(eventStart, eventEnd);

    int iterStart = params.indexOf("iterations=") + 11;
    int iterations = params.substring(iterStart).toInt();

    if (iterations <= 0 || iterations > 20) {
        iterations = 5; // Default to 5 iterations
    }

    _safetyTestActive = true;
    _currentSafetyTest = "response_" + eventType;

    String response = "{\"status\":\"started\",\"test\":\"response_time\",\"event\":\"" + eventType +
                     "\",\"iterations\":" + String(iterations) + "}";
    sendJsonResponse(client, response);

    // Perform multiple response time measurements
    unsigned long totalResponseTime = 0;
    int successCount = 0;

    for (int i = 0; i < iterations; i++) {
        // Reset to IDLE state before each test
        _stateMachine->processEvent(StateMachine::EVENT_STOP_PRESSED);
        delay(500); // Give time to stabilize

        // Start measurement
        _safetyTestStartTime = micros();

        // Trigger appropriate safety event
        bool success = false;
        if (eventType == "obstacle") {
            success = simulateObstacle(5.0, "front"); // Critical distance
        } else if (eventType == "user") {
            success = simulateUserDeparture();
        } else if (eventType == "stall") {
            success = simulateMotorStall();
        }

        // Wait for state change or timeout
        unsigned long startWait = millis();
        while (_stateMachine->getCurrentState() == StateMachine::STATE_IDLE &&
               millis() - startWait < 2000) {
            delay(10);
        }

        _safetyTestEndTime = micros();
        unsigned long responseTime = (_safetyTestEndTime - _safetyTestStartTime) / 1000; // Convert to ms

        // Log this iteration
        String iterDescription = "Response time test #" + String(i + 1) +
                                " for " + eventType + " event";
        logSafetyEvent("response", iterDescription,
                       success ? "SUCCESS" : "FAILED", responseTime);

        if (success) {
            totalResponseTime += responseTime;
            successCount++;
        }

        // Allow system to recover between tests
        delay(1000);
    }

    // Calculate average response time
    unsigned long avgResponseTime = (successCount > 0) ? totalResponseTime / successCount : 0;

    // Log summary
    String summaryDescription = "Response time test summary for " + eventType +
                              " event (" + String(successCount) + "/" + String(iterations) + " successful)";
    logSafetyEvent("response_summary", summaryDescription, "COMPLETE", avgResponseTime);

    _safetyTestActive = false;
    _currentSafetyTest = "";

    Serial.print("Response time test completed. Average: ");
    Serial.print(avgResponseTime);
    Serial.println(" ms");
}

// Run automated test sequence
void WebServer::runAutomatedTestSequence(WiFiClient& client) {
    _safetyTestActive = true;
    _currentSafetyTest = "automated_sequence";
    _safetyTestStartTime = millis();
    _safetyTestStep = 0;

    String response = "{\"status\":\"started\",\"test\":\"automated_sequence\"}";
    sendJsonResponse(client, response);

    // Log start of test sequence
    logSafetyEvent("automated_test", "Starting automated safety test sequence", "STARTED", 0);

    // Step 1: Front obstacle detection
    _safetyTestStep = 1;
    simulateObstacle(15.0, "front");
    delay(2000);
    _stateMachine->processEvent(StateMachine::EVENT_STOP_PRESSED); // Reset
    delay(1000);

    // Step 2: Rear obstacle detection
    _safetyTestStep = 2;
    simulateObstacle(15.0, "rear");
    delay(2000);
    _stateMachine->processEvent(StateMachine::EVENT_STOP_PRESSED); // Reset
    delay(1000);

    // Step 3: User departure detection
    _safetyTestStep = 3;
    // First put into swinging state
    _stateMachine->processEvent(StateMachine::EVENT_START_PRESSED);
    delay(1000);
    simulateUserDeparture();
    delay(2000);
    _stateMachine->processEvent(StateMachine::EVENT_STOP_PRESSED); // Reset
    delay(1000);

    // Step 4: Motor stall detection
    _safetyTestStep = 4;
    // First put into swinging state
    _stateMachine->processEvent(StateMachine::EVENT_START_PRESSED);
    delay(1000);
    simulateMotorStall();
    delay(2000);
    _stateMachine->processEvent(StateMachine::EVENT_STOP_PRESSED); // Reset
    delay(1000);

    // Step 5: Emergency response time
    _safetyTestStep = 5;
    measureResponseTime(client, "-response-time?event=obstacle&iterations=3");
    delay(1000);

    // Log completion of test sequence
    logSafetyEvent("automated_test", "Automated safety test sequence completed", "COMPLETED", 0);

    _safetyTestActive = false;
    _currentSafetyTest = "";

    Serial.println("Automated test sequence completed");
}

// Get current safety test status
void WebServer::getSafetyTestStatus(WiFiClient& client) {
    String response = "{";
    response += "\"active\":" + String(_safetyTestActive ? "true" : "false") + ",";
    response += "\"test_type\":\"" + _currentSafetyTest + "\",";
    response += "\"override_enabled\":" + String(_safetyOverrideEnabled ? "true" : "false") + ",";

    if (_safetyTestActive) {
        unsigned long elapsed = millis() - _safetyTestStartTime;
        response += "\"elapsed\":" + String(elapsed) + ",";
        response += "\"step\":" + String(_safetyTestStep) + ",";
        response += "\"threshold\":" + String(_safetyTestThreshold);
    } else {
        response += "\"elapsed\":0,\"step\":0,\"threshold\":0";
    }

    if (_safetyOverrideEnabled) {
        unsigned long remainingTime = (_safetyOverrideTimeout > millis()) ?
                                    (_safetyOverrideTimeout - millis()) / 1000 : 0;
        response += ",\"override_timeout\":" + String(remainingTime);
    }

    response += "}";
    sendJsonResponse(client, response);
}

// Get safety event logs
void WebServer::getSafetyEventLogs(WiFiClient& client) {
    String response = "{\"logs\":[";

    bool first = true;
    for (uint8_t i = 0; i < MAX_SAFETY_LOGS; i++) {
        if (_safetyEventLogs[i].timestamp > 0) {
            if (!first) {
                response += ",";
            }
            first = false;

            response += "{";
            response += "\"timestamp\":" + String(_safetyEventLogs[i].timestamp) + ",";
            response += "\"event\":\"" + _safetyEventLogs[i].eventType + "\",";
            response += "\"description\":\"" + _safetyEventLogs[i].description + "\",";
            response += "\"status\":\"" + _safetyEventLogs[i].status + "\",";
            response += "\"response_time\":" + String(_safetyEventLogs[i].responseTime);
            response += "}";
        }
    }

    response += "]}";
    sendJsonResponse(client, response);
}

// Toggle safety override mode
void WebServer::toggleSafetyOverride(WiFiClient& client, bool enable) {
    if (enable) {
        _safetyOverrideEnabled = true;
        _safetyOverrideTimeout = millis() + (5 * 60 * 1000); // 5 minute timeout

        // Log safety override
        logSafetyEvent("override", "Safety override mode enabled", "WARNING", 0);

        String response = "{\"status\":\"enabled\",\"override_timeout\":300}"; // 300 seconds
        sendJsonResponse(client, response);

        Serial.println("Safety override enabled for testing (5 minute timeout)");
    } else {
        _safetyOverrideEnabled = false;

        // Log safety override disabled
        logSafetyEvent("override", "Safety override mode disabled", "INFO", 0);

        sendJsonResponse(client, "{\"status\":\"disabled\"}");

        Serial.println("Safety override disabled");
    }
}

// Reset safety logs
void WebServer::resetSafetyLogs(WiFiClient& client) {
    for (uint8_t i = 0; i < MAX_SAFETY_LOGS; i++) {
        _safetyEventLogs[i].timestamp = 0;
    }
    _safetyLogIndex = 0;

    sendJsonResponse(client, "{\"status\":\"logs_reset\"}");
    Serial.println("Safety logs reset");
}

// Log a safety event
void WebServer::logSafetyEvent(String eventType, String description, String status, unsigned long responseTime) {
    // Store in circular buffer
    _safetyEventLogs[_safetyLogIndex].timestamp = millis();
    _safetyEventLogs[_safetyLogIndex].eventType = eventType;
    _safetyEventLogs[_safetyLogIndex].description = description;
    _safetyEventLogs[_safetyLogIndex].status = status;
    _safetyEventLogs[_safetyLogIndex].responseTime = responseTime;

    // Log to serial for debugging
    Serial.print("SAFETY LOG: [");
    Serial.print(eventType);
    Serial.print("] ");
    Serial.print(description);
    Serial.print(" - ");
    Serial.print(status);
    if (responseTime > 0) {
        Serial.print(" (");
        Serial.print(responseTime);
        Serial.print("ms)");
    }
    Serial.println();

    // Move to next log entry
    _safetyLogIndex = (_safetyLogIndex + 1) % MAX_SAFETY_LOGS;
}

// Simulate an obstacle at a specific distance
bool WebServer::simulateObstacle(float distance, String sensor) {
    if (!_safetyOverrideEnabled) {
        Serial.println("Safety override not enabled - cannot simulate obstacle");
        return false;
    }

    // This needs to be implemented in SafetyMonitor to allow
    // artificially setting sensor values for testing purposes
    // _safetyMonitor->simulateObstacleDetection(sensor, distance);

    // For now, we'll directly generate events to simulate this
    if (distance < CRITICAL_DISTANCE_CM) {
        _stateMachine->processEvent(StateMachine::EVENT_EMERGENCY);
        return true;
    } else if (distance < OBSTACLE_DISTANCE_CM) {
        _stateMachine->processEvent(StateMachine::EVENT_OBSTACLE_DETECTED);
        return true;
    }

    return false;
}

// Simulate user departure
bool WebServer::simulateUserDeparture() {
    if (!_safetyOverrideEnabled) {
        Serial.println("Safety override not enabled - cannot simulate user departure");
        return false;
    }

    // This would need to be implemented in SafetyMonitor
    // _safetyMonitor->simulateUserDeparture();

    // Direct event simulation
    _stateMachine->processEvent(StateMachine::EVENT_PRESSURE_OFF);
    return true;
}

// Simulate motor stall
bool WebServer::simulateMotorStall() {
    if (!_safetyOverrideEnabled) {
        Serial.println("Safety override not enabled - cannot simulate motor stall");
        return false;
    }

    // This would need to be implemented in SafetyMonitor
    // _safetyMonitor->simulateMotorStall();

    // We'll directly simulate a motor stall by forcibly incrementing
    // the stall count in SafetyMonitor - this needs to be added to
    // the SafetyMonitor class
    // _safetyMonitor->_stallCount = _safetyMonitor->_maxStallCount;
    // _safetyMonitor->updateMotorStatus(true, 0); // This would trigger the stall detection

    // For now, we'll directly trigger an emergency
    _stateMachine->processEvent(StateMachine::EVENT_EMERGENCY);
    return true;
}

// Send the safety test page
void WebServer::sendSafetyTestPage(WiFiClient& client) {
    sendHttpHeader(client);

    client.println("<!DOCTYPE html>");
    client.println("<html lang='en'>");
    client.println("<head>");
    client.println("<meta charset='UTF-8'>");
    client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
    client.println("<title>Safety Testing - Wheelchair Swing</title>");
    client.println("<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>");
    client.println("<style>");
    client.println("body { font-family: Arial; margin: 20px; background: #f5f5f5; }");
    client.println(".container { max-width: 1200px; margin: 0 auto; }");
    client.println(".card { background: white; padding: 20px; margin: 20px 0; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }");
    client.println(".safety-override { background: #f8d7da; color: #721c24; padding: 15px; border-radius: 5px; margin: 10px 0; }");
    client.println(".safety-override.active { background: #d4edda; color: #155724; }");
    client.println(".test-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; }");
    client.println(".slider-control { margin: 15px 0; }");
    client.println(".slider { width: 100%; margin: 10px 0; }");
    client.println(".button { background: #007bff; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; margin: 5px; }");
    client.println(".button:hover { background: #0056b3; }");
    client.println(".button.danger { background: #dc3545; }");
    client.println(".button.warning { background: #ffc107; color: #212529; }");
    client.println(".button.success { background: #28a745; }");
    client.println(".response-time { font-size: 24px; font-weight: bold; text-align: center; margin: 20px 0; }");
    client.println(".log-container { max-height: 400px; overflow-y: auto; border: 1px solid #ddd; padding: 10px; border-radius: 5px; }");
    client.println(".log-entry { padding: 8px; margin: 5px 0; border-radius: 5px; }");
    client.println(".log-info { background: #d1ecf1; color: #0c5460; }");
    client.println(".log-warning { background: #fff3cd; color: #856404; }");
    client.println(".log-error { background: #f8d7da; color: #721c24; }");
    client.println(".log-success { background: #d4edda; color: #155724; }");
    client.println(".progress-bar { background: #ddd; height: 20px; border-radius: 10px; margin: 20px 0; }");
    client.println(".progress-fill { background: #4CAF50; height: 100%; border-radius: 10px; width: 0%; transition: width 0.3s; }");
    client.println(".safety-zone { width: 100%; height: 200px; background: #eee; position: relative; border: 1px solid #ccc; border-radius: 5px; overflow: hidden; margin: 20px 0; }");
    client.println(".sensor { position: absolute; top: 50%; transform: translateY(-50%); width: 20px; height: 20px; background: #007bff; border-radius: 50%; }");
    client.println(".sensor.front { left: 10px; }");
    client.println(".sensor.rear { right: 10px; }");
    client.println(".swing { position: absolute; top: 50%; left: 50%; transform: translate(-50%, -50%); width: 80px; height: 40px; background: #ffc107; border-radius: 5px; }");
    client.println(".zone { position: absolute; top: 0; height: 100%; opacity: 0.3; }");
    client.println(".critical-zone { background: #f00; }");
    client.println(".warning-zone { background: #ffc107; }");
    client.println("</style>");
    client.println("</head>");
    client.println("<body>");

    client.println(generateSafetyTestHTML());

    client.println("<script>");
    client.println("let safetyTestActive = false;");
    client.println("let safetyOverrideEnabled = false;");
    client.println("let overrideTimeoutTimer;");
    client.println("let responseTimeMeasurements = [];");

    // JavaScript for safety testing functionality
    client.println("function toggleSafetyOverride(enable) {");
    client.println("  const endpoint = enable ? '/api/safety-test-override?enable=1' : '/api/safety-test-override?disable=1';");
    client.println("  fetch(endpoint)");
    client.println("    .then(response => response.json())");
    client.println("    .then(data => {");
    client.println("      safetyOverrideEnabled = enable;");
    client.println("      document.getElementById('safetyOverride').className = 'safety-override ' + (enable ? 'active' : '');");
    client.println("      document.getElementById('safetyOverride').innerHTML = enable ? ");
    client.println("        '⚠️ <strong>SAFETY OVERRIDE ENABLED</strong> - Safety systems are bypassed for testing! Auto-disables in <span id=\"overrideTimeout\">300</span> seconds.' : ");
    client.println("        '✓ Safety systems active - Override disabled';");
    client.println("      if (enable) {");
    client.println("        startOverrideCountdown(300);");
    client.println("        logSafetyEvent('override', 'Safety override enabled for testing', 'WARNING');");
    client.println("      } else {");
    client.println("        clearTimeout(overrideTimeoutTimer);");
    client.println("        logSafetyEvent('override', 'Safety override disabled', 'INFO');");
    client.println("      }");
    client.println("    });");
    client.println("}");

    client.println("function startOverrideCountdown(seconds) {");
    client.println("  document.getElementById('overrideTimeout').textContent = seconds;");
    client.println("  if (seconds <= 0) {");
    client.println("    toggleSafetyOverride(false);");
    client.println("    return;");
    client.println("  }");
    client.println("  overrideTimeoutTimer = setTimeout(() => startOverrideCountdown(seconds - 1), 1000);");
    client.println("}");

    client.println("function triggerSafetyEvent(event, params = {}) {");
    client.println("  if (!safetyOverrideEnabled) {");
    client.println("    alert('Safety override must be enabled for testing!');");
    client.println("    return;");
    client.println("  }");
    client.println("  let endpoint = `/api/safety-test-trigger?event=${event}`;");
    client.println("  if (event === 'obstacle') {");
    client.println("    const distance = params.distance || document.getElementById('obstacleDistance').value;");
    client.println("    const sensor = params.sensor || document.getElementById('obstacleSensor').value;");
    client.println("    endpoint += `&distance=${distance}&sensor=${sensor}`;");
    client.println("  }");
    client.println("  fetch(endpoint)");
    client.println("    .then(response => response.json())");
    client.println("    .then(data => {");
    client.println("      if (data.status === 'success') {");
    client.println("        logSafetyEvent(event, data.description, 'SUCCESS', data.response_time);");
    client.println("        document.getElementById('responseTime').textContent = data.response_time + ' ms';");
    client.println("      } else {");
    client.println("        logSafetyEvent(event, data.description || 'Event trigger failed', 'FAILED');");
    client.println("      }");
    client.println("    });");
    client.println("}");

    client.println("function runThresholdTest() {");
    client.println("  if (!safetyOverrideEnabled) {");
    client.println("    alert('Safety override must be enabled for testing!');");
    client.println("    return;");
    client.println("  }");
    client.println("  const sensor = document.getElementById('thresholdSensor').value;");
    client.println("  const startVal = document.getElementById('thresholdStart').value;");
    client.println("  const endVal = document.getElementById('thresholdEnd').value;");
    client.println("  const steps = document.getElementById('thresholdSteps').value;");
    client.println("  const endpoint = `/api/safety-test-threshold?sensor=${sensor}&start=${startVal}&end=${endVal}&steps=${steps}`;");
    client.println("  fetch(endpoint)");
    client.println("    .then(response => response.json())");
    client.println("    .then(data => {");
    client.println("      if (data.status === 'started') {");
    client.println("        safetyTestActive = true;");
    client.println("        logSafetyEvent('threshold', `Starting threshold test for ${sensor} from ${startVal}cm to ${endVal}cm`, 'STARTED');");
    client.println("        document.getElementById('progressBar').style.display = 'block';");
    client.println("        document.getElementById('progressFill').style.width = '0%';");
    client.println("        startProgressUpdate();");
    client.println("      }");
    client.println("    });");
    client.println("}");

    client.println("function startProgressUpdate() {");
    client.println("  let progress = 0;");
    client.println("  const interval = setInterval(() => {");
    client.println("    progress += 5;");
    client.println("    if (progress > 100) {");
    client.println("      clearInterval(interval);");
    client.println("      safetyTestActive = false;");
    client.println("      return;");
    client.println("    }");
    client.println("    document.getElementById('progressFill').style.width = progress + '%';");
    client.println("    // Check test status");
    client.println("    fetch('/api/safety-test-status')");
    client.println("      .then(response => response.json())");
    client.println("      .then(data => {");
    client.println("        if (!data.active) {");
    client.println("          clearInterval(interval);");
    client.println("          document.getElementById('progressFill').style.width = '100%';");
    client.println("          setTimeout(() => {");
    client.println("            document.getElementById('progressBar').style.display = 'none';");
    client.println("          }, 1000);");
    client.println("          logSafetyEvent('threshold', 'Threshold test completed', 'COMPLETED');");
    client.println("        }");
    client.println("      });");
    client.println("  }, 500);");
    client.println("}");

    client.println("function measureResponseTime() {");
    client.println("  if (!safetyOverrideEnabled) {");
    client.println("    alert('Safety override must be enabled for testing!');");
    client.println("    return;");
    client.println("  }");
    client.println("  const event = document.getElementById('responseEvent').value;");
    client.println("  const iterations = document.getElementById('responseIterations').value;");
    client.println("  const endpoint = `/api/safety-test-response-time?event=${event}&iterations=${iterations}`;");
    client.println("  responseTimeMeasurements = [];");
    client.println("  fetch(endpoint)");
    client.println("    .then(response => response.json())");
    client.println("    .then(data => {");
    client.println("      if (data.status === 'started') {");
    client.println("        safetyTestActive = true;");
    client.println("        logSafetyEvent('response', `Starting response time measurement for ${event} (${iterations} iterations)`, 'STARTED');");
    client.println("        document.getElementById('progressBar').style.display = 'block';");
    client.println("        document.getElementById('progressFill').style.width = '0%';");
    client.println("        startProgressUpdate();");
    client.println("      }");
    client.println("    });");
    client.println("}");

    client.println("function runAutomatedTest() {");
    client.println("  if (!safetyOverrideEnabled) {");
    client.println("    alert('Safety override must be enabled for testing!');");
    client.println("    return;");
    client.println("  }");
    client.println("  fetch('/api/safety-test-automated-test')");
    client.println("    .then(response => response.json())");
    client.println("    .then(data => {");
    client.println("      if (data.status === 'started') {");
    client.println("        safetyTestActive = true;");
    client.println("        logSafetyEvent('automated', 'Starting automated safety test sequence', 'STARTED');");
    client.println("        document.getElementById('progressBar').style.display = 'block';");
    client.println("        document.getElementById('progressFill').style.width = '0%';");
    client.println("        startProgressUpdate();");
    client.println("      }");
    client.println("    });");
    client.println("}");

    client.println("function updateSafetyZones() {");
    client.println("  const warningDistance = document.getElementById('thresholdStart').value;");
    client.println("  const criticalDistance = document.getElementById('thresholdEnd').value;");
    client.println("  const maxDistance = 100; // Maximum display distance in cm");
    client.println("  const zoneWidth = document.querySelector('.safety-zone').offsetWidth;");
    client.println("  ");
    client.println("  // Calculate pixel positions (scale from cm to pixels)");
    client.println("  const warningPixels = (warningDistance / maxDistance) * (zoneWidth / 2);");
    client.println("  const criticalPixels = (criticalDistance / maxDistance) * (zoneWidth / 2);");
    client.println("  ");
    client.println("  // Update front zones");
    client.println("  document.querySelector('.front-warning').style.left = '0';");
    client.println("  document.querySelector('.front-warning').style.width = warningPixels + 'px';");
    client.println("  document.querySelector('.front-critical').style.left = '0';");
    client.println("  document.querySelector('.front-critical').style.width = criticalPixels + 'px';");
    client.println("  ");
    client.println("  // Update rear zones");
    client.println("  document.querySelector('.rear-warning').style.right = '0';");
    client.println("  document.querySelector('.rear-warning').style.width = warningPixels + 'px';");
    client.println("  document.querySelector('.rear-critical').style.right = '0';");
    client.println("  document.querySelector('.rear-critical').style.width = criticalPixels + 'px';");
    client.println("}");

    client.println("function logSafetyEvent(type, description, status, responseTime = '') {");
    client.println("  const logContainer = document.getElementById('logContainer');");
    client.println("  const timestamp = new Date().toLocaleTimeString();");
    client.println("  ");
    client.println("  let logClass = 'log-info';");
    client.println("  if (status === 'WARNING' || status === 'STARTED') logClass = 'log-warning';");
    client.println("  else if (status === 'ERROR' || status === 'FAILED') logClass = 'log-error';");
    client.println("  else if (status === 'SUCCESS' || status === 'COMPLETED') logClass = 'log-success';");
    client.println("  ");
    client.println("  const logEntry = document.createElement('div');");
    client.println("  logEntry.className = `log-entry ${logClass}`;");
    client.println("  logEntry.innerHTML = `<strong>[${timestamp}]</strong> ${description} - <em>${status}</em> ${responseTime ? '(' + responseTime + 'ms)' : ''}`;");
    client.println("  ");
    client.println("  logContainer.insertBefore(logEntry, logContainer.firstChild);");
    client.println("  ");
    client.println("  // Limit to 100 log entries");
    client.println("  if (logContainer.children.length > 100) {");
    client.println("    logContainer.removeChild(logContainer.lastChild);");
    client.println("  }");
    client.println("}");

    client.println("function resetSafetyLogs() {");
    client.println("  if (confirm('Are you sure you want to clear all safety logs?')) {");
    client.println("    fetch('/api/safety-test-reset-logs')");
    client.println("      .then(response => response.json())");
    client.println("      .then(data => {");
    client.println("        document.getElementById('logContainer').innerHTML = '';");
    client.println("        logSafetyEvent('system', 'Safety logs cleared', 'INFO');");
    client.println("      });");
    client.println("  }");
    client.println("}");

    client.println("// Check safety status on load");
    client.println("document.addEventListener('DOMContentLoaded', function() {");
    client.println("  fetch('/api/safety-test-status')");
    client.println("    .then(response => response.json())");
    client.println("    .then(data => {");
    client.println("      safetyOverrideEnabled = data.override_enabled;");
    client.println("      document.getElementById('safetyOverride').className = 'safety-override ' + (safetyOverrideEnabled ? 'active' : '');");
    client.println("      if (safetyOverrideEnabled) {");
    client.println("        document.getElementById('safetyOverride').innerHTML = ");
    client.println("          '⚠️ <strong>SAFETY OVERRIDE ENABLED</strong> - Safety systems are bypassed for testing! Auto-disables in <span id=\"overrideTimeout\">' + data.override_timeout + '</span> seconds.';");
    client.println("        startOverrideCountdown(data.override_timeout);");
    client.println("      }");
    client.println("    });");
    client.println("  ");
    client.println("  // Load existing logs");
    client.println("  fetch('/api/safety-test-logs')");
    client.println("    .then(response => response.json())");
    client.println("    .then(data => {");
    client.println("      if (data.logs && data.logs.length > 0) {");
    client.println("        data.logs.forEach(log => {");
    client.println("          logSafetyEvent(log.event, log.description, log.status, log.response_time);");
    client.println("        });");
    client.println("      }");
    client.println("    });");
    client.println("  ");
    client.println("  // Initialize safety zones");
    client.println("  updateSafetyZones();");
    client.println("});");

    client.println("</script>");
    client.println("</body></html>");
}

String WebServer::generateSafetyTestHTML() {
    String html = "<div class='container'>";
    html += "<h1>Safety System Testing</h1>";
    html += "<p><a href='/'>← Back to Home</a></p>";

    // Safety Override Control
    html += "<div id='safetyOverride' class='safety-override'>";
    html += "✓ Safety systems active - Override disabled";
    html += "</div>";

    html += "<div class='card'>";
    html += "<h2>Safety Override Control</h2>";
    html += "<p>Warning: Enabling safety override will bypass normal safety protections for testing purposes.</p>";
    html += "<button class='button warning' onclick='toggleSafetyOverride(true)'>Enable Safety Override</button>";
    html += "<button class='button' onclick='toggleSafetyOverride(false)'>Disable Safety Override</button>";
    html += "</div>";

    // Safety Visualization
    html += "<div class='card'>";
    html += "<h2>Safety Zone Visualization</h2>";
    html += "<div class='safety-zone'>";
    html += "<div class='zone warning-zone front-warning'></div>";
    html += "<div class='zone critical-zone front-critical'></div>";
    html += "<div class='sensor front'></div>";
    html += "<div class='swing'></div>";
    html += "<div class='sensor rear'></div>";
    html += "<div class='zone warning-zone rear-warning'></div>";
    html += "<div class='zone critical-zone rear-critical'></div>";
    html += "</div>";
    html += "<p>Visual representation of safety thresholds: red = critical, yellow = warning</p>";
    html += "</div>";

    // Progress bar (hidden by default)
    html += "<div id='progressBar' class='progress-bar' style='display: none;'>";
    html += "<div id='progressFill' class='progress-fill'></div>";
    html += "</div>";

    // Event Trigger Section
    html += "<div class='card'>";
    html += "<h2>Manual Safety Event Triggers</h2>";
    html += "<div class='test-grid'>";

    // Obstacle Detection Trigger
    html += "<div>";
    html += "<h3>Obstacle Detection</h3>";
    html += "<div class='slider-control'>";
    html += "<label>Distance (cm): <span id='obstacleDistanceDisplay'>30</span></label>";
    html += "<input type='range' id='obstacleDistance' class='slider' min='5' max='100' value='30' oninput='document.getElementById(\"obstacleDistanceDisplay\").textContent=this.value'>";
    html += "</div>";
    html += "<div class='slider-control'>";
    html += "<label>Sensor:</label>";
    html += "<select id='obstacleSensor'>";
    html += "<option value='front'>Front</option>";
    html += "<option value='rear'>Rear</option>";
    html += "</select>";
    html += "</div>";
    html += "<button class='button danger' onclick='triggerSafetyEvent(\"obstacle\")'>Simulate Obstacle</button>";
    html += "</div>";

    // User Departure Trigger
    html += "<div>";
    html += "<h3>User Presence</h3>";
    html += "<p>Simulate user leaving the swing while in operation.</p>";
    html += "<button class='button danger' onclick='triggerSafetyEvent(\"user\")'>Simulate User Departure</button>";
    html += "</div>";

    html += "</div>";

    // Second row
    html += "<div class='test-grid'>";

    // Motor Stall Trigger
    html += "<div>";
    html += "<h3>Motor Stall</h3>";
    html += "<p>Simulate motor stall condition during operation.</p>";
    html += "<button class='button danger' onclick='triggerSafetyEvent(\"stall\")'>Simulate Motor Stall</button>";
    html += "</div>";

    // Response Time Display
    html += "<div>";
    html += "<h3>Response Time</h3>";
    html += "<div class='response-time' id='responseTime'>-- ms</div>";
    html += "<p>Time from trigger to emergency stop</p>";
    html += "</div>";

    html += "</div>";
    html += "</div>";

    // Threshold Testing
    html += "<div class='card'>";
    html += "<h2>Threshold Testing</h2>";
    html += "<p>Gradually decrease distance to determine precise triggering thresholds.</p>";

    html += "<div class='slider-control'>";
    html += "<label>Sensor:</label>";
    html += "<select id='thresholdSensor'>";
    html += "<option value='front'>Front</option>";
    html += "<option value='rear'>Rear</option>";
    html += "</select>";
    html += "</div>";

    html += "<div class='slider-control'>";
    html += "<label>Start Distance (cm): <span id='thresholdStartDisplay'>100</span></label>";
    html += "<input type='range' id='thresholdStart' class='slider' min='50' max='100' value='100' oninput='document.getElementById(\"thresholdStartDisplay\").textContent=this.value; updateSafetyZones();'>";
    html += "</div>";

    html += "<div class='slider-control'>";
    html += "<label>End Distance (cm): <span id='thresholdEndDisplay'>5</span></label>";
    html += "<input type='range' id='thresholdEnd' class='slider' min='5' max='30' value='5' oninput='document.getElementById(\"thresholdEndDisplay\").textContent=this.value; updateSafetyZones();'>";
    html += "</div>";

    html += "<div class='slider-control'>";
    html += "<label>Steps: <span id='thresholdStepsDisplay'>20</span></label>";
    html += "<input type='range' id='thresholdSteps' class='slider' min='5' max='50' value='20' oninput='document.getElementById(\"thresholdStepsDisplay\").textContent=this.value'>";
    html += "</div>";

    html += "<button class='button warning' onclick='runThresholdTest()'>Run Threshold Test</button>";
    html += "</div>";

    // Response Time Testing
    html += "<div class='card'>";
    html += "<h2>Response Time Testing</h2>";
    html += "<p>Measure how quickly the system responds to safety events.</p>";

    html += "<div class='slider-control'>";
    html += "<label>Event Type:</label>";
    html += "<select id='responseEvent'>";
    html += "<option value='obstacle'>Obstacle Detection</option>";
    html += "<option value='user'>User Departure</option>";
    html += "<option value='stall'>Motor Stall</option>";
    html += "</select>";
    html += "</div>";

    html += "<div class='slider-control'>";
    html += "<label>Iterations: <span id='responseIterationsDisplay'>5</span></label>";
    html += "<input type='range' id='responseIterations' class='slider' min='1' max='10' value='5' oninput='document.getElementById(\"responseIterationsDisplay\").textContent=this.value'>";
    html += "</div>";

    html += "<button class='button warning' onclick='measureResponseTime()'>Measure Response Time</button>";
    html += "</div>";

    // Automated Test Sequence
    html += "<div class='card'>";
    html += "<h2>Automated Test Sequence</h2>";
    html += "<p>Run a comprehensive suite of safety tests automatically.</p>";
    html += "<button class='button success' onclick='runAutomatedTest()'>Run Automated Test Sequence</button>";
    html += "</div>";

    // Safety Event Log
    html += "<div class='card'>";
    html += "<h2>Safety Event Log</h2>";
    html += "<div class='log-container' id='logContainer'>";
    html += "<!-- Log entries will be added here via JavaScript -->";
    html += "</div>";
    html += "<button class='button' onclick='resetSafetyLogs()'>Clear Log</button>";
    html += "</div>";

    html += "</div>";
    return html;
}

void WebServer::handleMotorTestAPI(WiFiClient& client, String command) {
    Serial.print("Motor Test API Command: ");
    Serial.println(command);

    // Safety check before any motor operation
    if (!checkMotorTestSafety() && !command.startsWith("-stop")) {
        sendJsonResponse(client, "{\"error\":\"Safety interlock active - obstacles detected\"}");
        return;
    }

    if (command.startsWith("-left")) {
        testMotorLeft(client, command);
    } else if (command.startsWith("-right")) {
        testMotorRight(client, command);
    } else if (command.startsWith("-ramp-test")) {
        startRampTest(client, command);
    } else if (command == "-position") {
        getMotorPosition(client);
    } else if (command.startsWith("-direction")) {
        testMotorDirection(client, command);
    } else if (command == "-sync-test") {
        testMotorSync(client);
    } else if (command == "-stop") {
        stopMotorTest(client);
    } else if (command == "-status") {
        getMotorTestStatus(client);
    } else if (command == "-reset-position") {
        resetMotorPositions();
        sendJsonResponse(client, "{\"status\":\"positions_reset\"}");
    } else {
        sendJsonResponse(client, "{\"error\":\"Unknown motor test command\"}");
    }
}

void WebServer::testMotorLeft(WiFiClient& client, String params) {
    // Extract parameters: -left?speed=300&steps=200&direction=1
    int speedStart = params.indexOf("speed=") + 6;
    int speedEnd = params.indexOf("&", speedStart);
    int speed = params.substring(speedStart, speedEnd).toInt();

    int stepsStart = params.indexOf("steps=") + 6;
    int stepsEnd = params.indexOf("&", stepsStart);
    int steps = params.substring(stepsStart, stepsEnd).toInt();

    int dirStart = params.indexOf("direction=") + 10;
    bool clockwise = params.substring(dirStart).toInt() == 1;

    if (speed < 100 || speed > 700) {
        sendJsonResponse(client, "{\"error\":\"Speed must be between 100-700 RPM\"}");
        return;
    }

    _motorTestActive = true;
    _currentMotorTest = "left";
    _motorTestStartTime = millis();

    // Command left motor through state machine or direct control
    // This would integrate with your StepperDriver
    _leftMotorPosition += clockwise ? steps : -steps;

    String response = "{\"status\":\"testing\",\"motor\":\"left\",\"speed\":" +
                     String(speed) + ",\"steps\":" + String(steps) +
                     ",\"direction\":\"" + (clockwise ? "CW" : "CCW") + "\"}";
    sendJsonResponse(client, response);

    Serial.print("Testing left motor: ");
    Serial.print(speed);
    Serial.print(" RPM, ");
    Serial.print(steps);
    Serial.println(" steps");
}

void WebServer::testMotorRight(WiFiClient& client, String params) {
    // Similar implementation to testMotorLeft but for right motor
    int speedStart = params.indexOf("speed=") + 6;
    int speedEnd = params.indexOf("&", speedStart);
    int speed = params.substring(speedStart, speedEnd).toInt();

    int stepsStart = params.indexOf("steps=") + 6;
    int stepsEnd = params.indexOf("&", stepsStart);
    int steps = params.substring(stepsStart, stepsEnd).toInt();

    int dirStart = params.indexOf("direction=") + 10;
    bool clockwise = params.substring(dirStart).toInt() == 1;

    if (speed < 100 || speed > 700) {
        sendJsonResponse(client, "{\"error\":\"Speed must be between 100-700 RPM\"}");
        return;
    }

    _motorTestActive = true;
    _currentMotorTest = "right";
    _motorTestStartTime = millis();

    _rightMotorPosition += clockwise ? steps : -steps;

    String response = "{\"status\":\"testing\",\"motor\":\"right\",\"speed\":" +
                     String(speed) + ",\"steps\":" + String(steps) +
                     ",\"direction\":\"" + (clockwise ? "CW" : "CCW") + "\"}";
    sendJsonResponse(client, response);

    Serial.print("Testing right motor: ");
    Serial.print(speed);
    Serial.print(" RPM, ");
    Serial.print(steps);
    Serial.println(" steps");
}

void WebServer::startRampTest(WiFiClient& client, String params) {
    // Extract motor parameter: -ramp-test?motor=left&max_speed=600
    int motorStart = params.indexOf("motor=") + 6;
    int motorEnd = params.indexOf("&", motorStart);
    String motor = params.substring(motorStart, motorEnd);

    int maxSpeedStart = params.indexOf("max_speed=") + 10;
    int maxSpeed = params.substring(maxSpeedStart).toInt();

    if (maxSpeed < 200 || maxSpeed > 700) {
        sendJsonResponse(client, "{\"error\":\"Max speed must be between 200-700 RPM\"}");
        return;
    }

    _motorTestActive = true;
    _currentMotorTest = "ramp_" + motor;
    _motorTestStartTime = millis();
    _motorTestStep = 100; // Starting speed

    String response = "{\"status\":\"ramp_started\",\"motor\":\"" + motor +
                     "\",\"max_speed\":" + String(maxSpeed) +
                     ",\"current_speed\":100}";
    sendJsonResponse(client, response);

    Serial.print("Starting ramp test for ");
    Serial.print(motor);
    Serial.print(" motor, max speed: ");
    Serial.println(maxSpeed);
}

void WebServer::testMotorDirection(WiFiClient& client, String params) {
    // Extract motor parameter: -direction?motor=left
    int motorStart = params.indexOf("motor=") + 6;
    String motor = params.substring(motorStart);

    _motorTestActive = true;
    _currentMotorTest = "direction_" + motor;
    _motorTestStartTime = millis();

    // 360 degree test = 200 steps for typical stepper (1.8° per step)
    int fullRotationSteps = 200;

    // Test clockwise rotation
    if (motor == "left") {
        _leftMotorPosition += fullRotationSteps;
    } else {
        _rightMotorPosition += fullRotationSteps;
    }

    String response = "{\"status\":\"direction_test\",\"motor\":\"" + motor +
                     "\",\"phase\":\"clockwise\",\"steps\":" + String(fullRotationSteps) + "}";
    sendJsonResponse(client, response);

    Serial.print("Testing direction for ");
    Serial.print(motor);
    Serial.println(" motor - 360° CW then CCW");
}

void WebServer::testMotorSync(WiFiClient& client) {
    _motorTestActive = true;
    _currentMotorTest = "synchronization";
    _motorTestStartTime = millis();

    // Test both motors at same speed for synchronization
    int testSpeed = 400; // RPM
    int testSteps = 100;

    String response = "{\"status\":\"sync_test\",\"speed\":" + String(testSpeed) +
                     ",\"steps\":" + String(testSteps) + "}";
    sendJsonResponse(client, response);

    Serial.println("Starting motor synchronization test");
}

void WebServer::getMotorPosition(WiFiClient& client) {
    String response = "{";
    response += "\"left_position\":" + String(_leftMotorPosition) + ",";
    response += "\"right_position\":" + String(_rightMotorPosition) + ",";
    response += "\"left_degrees\":" + String(_leftMotorPosition * 1.8) + ",";
    response += "\"right_degrees\":" + String(_rightMotorPosition * 1.8);
    response += "}";

    sendJsonResponse(client, response);
}

void WebServer::stopMotorTest(WiFiClient& client) {
    _motorTestActive = false;
    _currentMotorTest = "";

    // Send stop commands to motors through state machine
    // _stateMachine->processEvent(StateMachine::EVENT_STOP_PRESSED);

    sendJsonResponse(client, "{\"status\":\"stopped\"}");
    Serial.println("Motor test stopped");
}

void WebServer::getMotorTestStatus(WiFiClient& client) {
    String response = "{";
    response += "\"active\":" + String(_motorTestActive ? "true" : "false") + ",";
    response += "\"test_type\":\"" + _currentMotorTest + "\",";
    response += "\"safety_ok\":" + String(checkMotorTestSafety() ? "true" : "false") + ",";

    if (_motorTestActive) {
        unsigned long elapsed = millis() - _motorTestStartTime;
        response += "\"elapsed\":" + String(elapsed) + ",";
        response += "\"step\":" + String(_motorTestStep);
    } else {
        response += "\"elapsed\":0,\"step\":0";
    }

    response += "}";
    sendJsonResponse(client, response);
}

bool WebServer::checkMotorTestSafety() {
    // Check for obstacles within 15cm safety zone
    float frontDist = _safetyMonitor->getFrontDistance();
    float rearDist = _safetyMonitor->getRearDistance();

    bool safetyOK = (frontDist == 0 || frontDist > 15.0) &&
                    (rearDist == 0 || rearDist > 15.0);

    if (!safetyOK && _motorTestSafetyCheck) {
        Serial.println("Motor test safety interlock: Obstacles detected within 15cm");
        return false;
    }

    return true;
}

void WebServer::resetMotorPositions() {
    _leftMotorPosition = 0;
    _rightMotorPosition = 0;
    Serial.println("Motor positions reset to home (0,0)");
}

void WebServer::sendMotorTestPage(WiFiClient& client) {
    sendHttpHeader(client);

    client.println("<!DOCTYPE html>");
    client.println("<html lang='en'>");
    client.println("<head>");
    client.println("<meta charset='UTF-8'>");
    client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
    client.println("<title>Motor Testing - Wheelchair Swing</title>");
    client.println("<style>");
    client.println("body { font-family: Arial; margin: 20px; background: #f5f5f5; }");
    client.println(".container { max-width: 1200px; margin: 0 auto; }");
    client.println(".card { background: white; padding: 20px; margin: 20px 0; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }");
    client.println(".motor-controls { display: grid; grid-template-columns: 1fr 1fr; gap: 20px; }");
    client.println(".slider-control { margin: 15px 0; }");
    client.println(".slider { width: 100%; margin: 10px 0; }");
    client.println(".button { background: #007bff; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; margin: 5px; }");
    client.println(".button:hover { background: #0056b3; }");
    client.println(".button.success { background: #28a745; }");
    client.println(".button.danger { background: #dc3545; }");
    client.println(".position-display { font-size: 18px; font-weight: bold; text-align: center; margin: 15px 0; }");
    client.println(".safety-status { padding: 10px; border-radius: 5px; margin: 10px 0; font-weight: bold; }");
    client.println(".safety-ok { background: #d4edda; color: #155724; }");
    client.println(".safety-warning { background: #f8d7da; color: #721c24; }");
    client.println("</style>");
    client.println("</head>");
    client.println("<body>");

    client.println(generateMotorTestHTML());

    client.println("<script>");
    client.println("let motorTestActive = false;");
    client.println("let leftPosition = 0;");
    client.println("let rightPosition = 0;");

    // JavaScript for motor control functionality
    client.println("function testMotor(motor) {");
    client.println("  const speed = document.getElementById(motor + 'Speed').value;");
    client.println("  const steps = document.getElementById(motor + 'Steps').value;");
    client.println("  const direction = document.getElementById(motor + 'Direction').checked ? 1 : 0;");
    client.println("  fetch(`/api/motor-test-${motor}?speed=${speed}&steps=${steps}&direction=${direction}`)");
    client.println("    .then(response => response.json())");
    client.println("    .then(data => updateStatus(data));");
    client.println("}");

    client.println("function startRampTest(motor) {");
    client.println("  const maxSpeed = document.getElementById('rampMaxSpeed').value;");
    client.println("  fetch(`/api/motor-test-ramp-test?motor=${motor}&max_speed=${maxSpeed}`)");
    client.println("    .then(response => response.json())");
    client.println("    .then(data => updateStatus(data));");
    client.println("}");

    client.println("function updatePosition() {");
    client.println("  fetch('/api/motor-test-position')");
    client.println("    .then(response => response.json())");
    client.println("    .then(data => {");
    client.println("      document.getElementById('leftPos').textContent = data.left_position;");
    client.println("      document.getElementById('rightPos').textContent = data.right_position;");
    client.println("      document.getElementById('leftDeg').textContent = data.left_degrees.toFixed(1);");
    client.println("      document.getElementById('rightDeg').textContent = data.right_degrees.toFixed(1);");
    client.println("    });");
    client.println("}");

    client.println("function stopAllMotors() {");
    client.println("  fetch('/api/motor-test-stop')");
    client.println("    .then(response => response.json())");
    client.println("    .then(data => updateStatus(data));");
    client.println("}");

    client.println("function updateStatus(data) {");
    client.println("  console.log('Motor test status:', data);");
    client.println("}");

    client.println("// Update position every 2 seconds");
    client.println("setInterval(updatePosition, 2000);");
    client.println("updatePosition();");

    client.println("</script>");
    client.println("</body></html>");
}

String WebServer::generateMotorTestHTML() {
    String html = "<div class='container'>";
    html += "<h1>Motor Testing Interface</h1>";
    html += "<p><a href='/'>← Back to Home</a></p>";

    // Safety status
    html += "<div class='safety-status " + String(checkMotorTestSafety() ? "safety-ok" : "safety-warning") + "'>";
    html += checkMotorTestSafety() ? "✓ Safety Systems OK - Testing Enabled" : "⚠ Safety Interlock Active - Obstacles Detected";
    html += "</div>";

    // Position display
    html += "<div class='card'>";
    html += "<h2>Current Positions</h2>";
    html += "<div class='motor-controls'>";
    html += "<div>";
    html += "<h3>Left Motor</h3>";
    html += "<div class='position-display'>Steps: <span id='leftPos'>0</span></div>";
    html += "<div class='position-display'>Degrees: <span id='leftDeg'>0.0</span>°</div>";
    html += "</div>";
    html += "<div>";
    html += "<h3>Right Motor</h3>";
    html += "<div class='position-display'>Steps: <span id='rightPos'>0</span></div>";
    html += "<div class='position-display'>Degrees: <span id='rightDeg'>0.0</span>°</div>";
    html += "</div>";
    html += "</div>";
    html += "</div>";

    // Individual motor controls
    html += "<div class='card'>";
    html += "<h2>Individual Motor Testing</h2>";
    html += "<div class='motor-controls'>";

    // Left motor controls
    html += "<div>";
    html += "<h3>Left Motor</h3>";
    html += "<div class='slider-control'>";
    html += "<label>Speed (RPM): <span id='leftSpeedDisplay'>300</span></label>";
    html += "<input type='range' id='leftSpeed' class='slider' min='100' max='700' value='300' oninput='document.getElementById(\"leftSpeedDisplay\").textContent=this.value'>";
    html += "</div>";
    html += "<div class='slider-control'>";
    html += "<label>Steps: <span id='leftStepsDisplay'>100</span></label>";
    html += "<input type='range' id='leftSteps' class='slider' min='10' max='500' value='100' oninput='document.getElementById(\"leftStepsDisplay\").textContent=this.value'>";
    html += "</div>";
    html += "<div class='slider-control'>";
    html += "<label><input type='checkbox' id='leftDirection'> Clockwise</label>";
    html += "</div>";
    html += "<button class='button' onclick='testMotor(\"left\")'>Test Left Motor</button>";
    html += "</div>";

    // Right motor controls
    html += "<div>";
    html += "<h3>Right Motor</h3>";
    html += "<div class='slider-control'>";
    html += "<label>Speed (RPM): <span id='rightSpeedDisplay'>300</span></label>";
    html += "<input type='range' id='rightSpeed' class='slider' min='100' max='700' value='300' oninput='document.getElementById(\"rightSpeedDisplay\").textContent=this.value'>";
    html += "</div>";
    html += "<div class='slider-control'>";
    html += "<label>Steps: <span id='rightStepsDisplay'>100</span></label>";
    html += "<input type='range' id='rightSteps' class='slider' min='10' max='500' value='100' oninput='document.getElementById(\"rightStepsDisplay\").textContent=this.value'>";
    html += "</div>";
    html += "<div class='slider-control'>";
    html += "<label><input type='checkbox' id='rightDirection'> Clockwise</label>";
    html += "</div>";
    html += "<button class='button' onclick='testMotor(\"right\")'>Test Right Motor</button>";
    html += "</div>";

    html += "</div>";
    html += "</div>";

    // Advanced testing
    html += "<div class='card'>";
    html += "<h2>Advanced Testing</h2>";
    html += "<div class='slider-control'>";
    html += "<label>Ramp Test Max Speed: <span id='rampMaxSpeedDisplay'>600</span> RPM</label>";
    html += "<input type='range' id='rampMaxSpeed' class='slider' min='200' max='700' value='600' oninput='document.getElementById(\"rampMaxSpeedDisplay\").textContent=this.value'>";
    html += "</div>";
    html += "<button class='button' onclick='startRampTest(\"left\")'>Ramp Test Left</button>";
    html += "<button class='button' onclick='startRampTest(\"right\")'>Ramp Test Right</button>";
    html += "<button class='button' onclick='fetch(\"/api/motor-test-direction?motor=left\")'>Direction Test Left</button>";
    html += "<button class='button' onclick='fetch(\"/api/motor-test-direction?motor=right\")'>Direction Test Right</button>";
    html += "<button class='button' onclick='fetch(\"/api/motor-test-sync-test\")'>Synchronization Test</button>";
    html += "</div>";

    // Emergency controls
    html += "<div class='card'>";
    html += "<h2>Emergency Controls</h2>";
    html += "<button class='button danger' onclick='stopAllMotors()'>EMERGENCY STOP</button>";
    html += "<button class='button' onclick='fetch(\"/api/motor-test-reset-position\").then(() => updatePosition())'>Reset Positions</button>";
    html += "</div>";

    html += "</div>";
    return html;
}

void WebServer::handleCalibrationAPI(WiFiClient& client, String command) {
    Serial.print("Calibration API Command: ");
    Serial.println(command);

    if (command.startsWith("-start")) {
        // Extract sensor type from command like "-start?sensor=ultrasonic1"
        int paramStart = command.indexOf("sensor=") + 7;
        String sensorType = command.substring(paramStart);
        startCalibration(client, sensorType);
    } else if (command == "-save") {
        saveCalibration(client);
    } else if (command == "-reset") {
        resetCalibration(client);
    } else if (command == "-status") {
        getCalibrationStatus(client);
    } else if (command.startsWith("-threshold")) {
        updateThreshold(client, command);
    } else if (command == "-data") {
        getCalibrationData(client);
    } else {
        sendJsonResponse(client, "{\"error\":\"Unknown calibration command\"}");
    }
}

void WebServer::startCalibration(WiFiClient& client, String sensorType) {
    if (_calibrationActive) {
        sendJsonResponse(client, "{\"error\":\"Calibration already in progress\"}");
        return;
    }

    _calibrationActive = true;
    _calibrationStartTime = millis();
    _calibrationStep = 1;
    _currentSensorCalibrating = sensorType;

    // Enter calibration mode in SafetyMonitor
    _safetyMonitor->enterCalibrationMode(sensorType);

    String response = "{\"status\":\"started\",\"sensor\":\"" + sensorType + "\",\"step\":1}";
    sendJsonResponse(client, response);

    Serial.print("Calibration started for sensor: ");
    Serial.println(sensorType);
}

void WebServer::saveCalibration(WiFiClient& client) {
    if (!_calibrationActive) {
        sendJsonResponse(client, "{\"error\":\"No calibration in progress\"}");
        return;
    }

    // Save calibration data from SafetyMonitor
    bool success = _safetyMonitor->saveCalibrationData();

    _calibrationActive = false;
    _safetyMonitor->exitCalibrationMode();

    if (success) {
        sendJsonResponse(client, "{\"status\":\"saved\",\"message\":\"Calibration saved successfully\"}");
        Serial.println("Calibration data saved successfully");
    } else {
        sendJsonResponse(client, "{\"error\":\"Failed to save calibration data\"}");
        Serial.println("Failed to save calibration data");
    }
}

void WebServer::resetCalibration(WiFiClient& client) {
    _calibrationActive = false;
    _safetyMonitor->exitCalibrationMode();
    _safetyMonitor->resetCalibrationData();

    sendJsonResponse(client, "{\"status\":\"reset\",\"message\":\"Calibration reset to defaults\"}");
    Serial.println("Calibration reset to factory defaults");
}

void WebServer::getCalibrationStatus(WiFiClient& client) {
    String response = "{";
    response += "\"active\":" + String(_calibrationActive ? "true" : "false") + ",";
    response += "\"step\":" + String(_calibrationStep) + ",";
    response += "\"sensor\":\"" + _currentSensorCalibrating + "\",";

    if (_calibrationActive) {
        unsigned long elapsed = millis() - _calibrationStartTime;
        response += "\"elapsed\":" + String(elapsed) + ",";

        // Get current calibration data from SafetyMonitor
        auto calibData = _safetyMonitor->getCurrentCalibrationData();
        response += "\"readings\":" + String(calibData.readingCount) + ",";
        response += "\"min\":" + String(calibData.minValue) + ",";
        response += "\"max\":" + String(calibData.maxValue) + ",";
        response += "\"average\":" + String(calibData.average);
    } else {
        response += "\"elapsed\":0,\"readings\":0,\"min\":0,\"max\":0,\"average\":0";
    }

    response += "}";
    sendJsonResponse(client, response);
}

void WebServer::updateThreshold(WiFiClient& client, String params) {
    // Parse threshold update parameters
    // Expected format: -threshold?sensor=ultrasonic1&min=10&max=100

    int sensorStart = params.indexOf("sensor=") + 7;
    int sensorEnd = params.indexOf("&", sensorStart);
    String sensor = params.substring(sensorStart, sensorEnd);

    int minStart = params.indexOf("min=") + 4;
    int minEnd = params.indexOf("&", minStart);
    float minValue = params.substring(minStart, minEnd).toFloat();

    int maxStart = params.indexOf("max=") + 4;
    float maxValue = params.substring(maxStart).toFloat();

    bool success = _safetyMonitor->updateSensorThresholds(sensor, minValue, maxValue);

    if (success) {
        String response = "{\"status\":\"updated\",\"sensor\":\"" + sensor +
                         "\",\"min\":" + String(minValue) + ",\"max\":" + String(maxValue) + "}";
        sendJsonResponse(client, response);
    } else {
        sendJsonResponse(client, "{\"error\":\"Failed to update thresholds\"}");
    }
}

void WebServer::getCalibrationData(WiFiClient& client) {
    // Get all calibration data for export
    String response = "{";
    response += "\"ultrasonic1\":{";
    response += "\"min\":" + String(_safetyMonitor->getUltrasonicMinThreshold(1)) + ",";
    response += "\"max\":" + String(_safetyMonitor->getUltrasonicMaxThreshold(1)) + ",";
    response += "\"baseline\":" + String(_safetyMonitor->getUltrasonicBaseline(1));
    response += "},";
    response += "\"ultrasonic2\":{";
    response += "\"min\":" + String(_safetyMonitor->getUltrasonicMinThreshold(2)) + ",";
    response += "\"max\":" + String(_safetyMonitor->getUltrasonicMaxThreshold(2)) + ",";
    response += "\"baseline\":" + String(_safetyMonitor->getUltrasonicBaseline(2));
    response += "},";
    response += "\"pressure\":{";
    response += "\"threshold\":" + String(_safetyMonitor->getPressureThreshold()) + ",";
    response += "\"baseline\":" + String(_safetyMonitor->getPressureBaseline());
    response += "},";
    response += "\"timestamp\":" + String(millis());
    response += "}";

    sendJsonResponse(client, response);
}

void WebServer::sendCalibrationPage(WiFiClient& client) {
    sendHttpHeader(client);

    client.println("<!DOCTYPE html>");
    client.println("<html lang='en'>");
    client.println("<head>");
    client.println("<meta charset='UTF-8'>");
    client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
    client.println("<title>Sensor Calibration - Wheelchair Swing</title>");
    client.println("<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>");
    client.println("<style>");
    client.println("body { font-family: Arial; margin: 20px; background: #f5f5f5; }");
    client.println(".container { max-width: 1000px; margin: 0 auto; }");
    client.println(".card { background: white; padding: 20px; margin: 20px 0; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }");
    client.println(".wizard-step { display: none; }");
    client.println(".wizard-step.active { display: block; }");
    client.println(".progress-bar { background: #ddd; height: 20px; border-radius: 10px; margin: 20px 0; }");
    client.println(".progress-fill { background: #4CAF50; height: 100%; border-radius: 10px; transition: width 0.3s; }");
    client.println(".threshold-slider { width: 100%; margin: 10px 0; }");
    client.println(".button { background: #007bff; color: white; padding: 10px 20px; border: none; border-radius: 5px; cursor: pointer; margin: 5px; }");
    client.println(".button:hover { background: #0056b3; }");
    client.println(".button.success { background: #28a745; }");
    client.println(".button.danger { background: #dc3545; }");
    client.println(".sensor-reading { font-size: 24px; font-weight: bold; text-align: center; margin: 20px 0; }");
    client.println(".calibration-status { padding: 15px; border-radius: 5px; margin: 10px 0; }");
    client.println(".status-ok { background: #d4edda; color: #155724; }");
    client.println(".status-warning { background: #fff3cd; color: #856404; }");
    client.println(".status-error { background: #f8d7da; color: #721c24; }");
    client.println("</style>");
    client.println("</head>");
    client.println("<body>");

    client.println(generateCalibrationWizardHTML());

    client.println("<script>");
    client.println("let calibrationChart;");
    client.println("let isCalibrating = false;");
    client.println("let currentStep = 1;");
    client.println("let calibrationData = [];");

    // JavaScript for calibration functionality
    client.println("function initializeChart() {");
    client.println("  const ctx = document.getElementById('calibrationChart').getContext('2d');");
    client.println("  calibrationChart = new Chart(ctx, {");
    client.println("    type: 'line',");
    client.println("    data: {");
    client.println("      labels: [],");
    client.println("      datasets: [{");
    client.println("        label: 'Sensor Reading',");
    client.println("        data: [],");
    client.println("        borderColor: '#007bff',");
    client.println("        tension: 0.1");
    client.println("      }]");
    client.println("    },");
    client.println("    options: { responsive: true, maintainAspectRatio: false }");
    client.println("  });");
    client.println("}");

    client.println("function startCalibration(sensor) {");
    client.println("  fetch('/api/calibrate-start?sensor=' + sensor)");
    client.println("    .then(response => response.json())");
    client.println("    .then(data => {");
    client.println("      if (data.status === 'started') {");
    client.println("        isCalibrating = true;");
    client.println("        showWizardStep(2);");
    client.println("        updateCalibrationStatus();");
    client.println("      }");
    client.println("    });");
    client.println("}");

    client.println("function updateCalibrationStatus() {");
    client.println("  if (!isCalibrating) return;");
    client.println("  fetch('/api/calibrate-status')");
    client.println("    .then(response => response.json())");
    client.println("    .then(data => {");
    client.println("      document.getElementById('readingCount').textContent = data.readings;");
    client.println("      document.getElementById('minValue').textContent = data.min.toFixed(2);");
    client.println("      document.getElementById('maxValue').textContent = data.max.toFixed(2);");
    client.println("      document.getElementById('avgValue').textContent = data.average.toFixed(2);");
    client.println("      updateProgress(data.readings);");
    client.println("      if (data.readings >= 100) enableSaveButton();");
    client.println("      setTimeout(updateCalibrationStatus, 1000);");
    client.println("    });");
    client.println("}");

    client.println("function updateProgress(readings) {");
    client.println("  const progress = Math.min(readings / 100 * 100, 100);");
    client.println("  document.getElementById('progressFill').style.width = progress + '%';");
    client.println("}");

    client.println("function enableSaveButton() {");
    client.println("  document.getElementById('saveBtn').disabled = false;");
    client.println("  document.getElementById('saveBtn').className = 'button success';");
    client.println("}");

    client.println("function saveCalibration() {");
    client.println("  fetch('/api/calibrate-save')");
    client.println("    .then(response => response.json())");
    client.println("    .then(data => {");
    client.println("      if (data.status === 'saved') {");
    client.println("        isCalibrating = false;");
    client.println("        showWizardStep(4);");
    client.println("      }");
    client.println("    });");
    client.println("}");

    client.println("function showWizardStep(step) {");
    client.println("  document.querySelectorAll('.wizard-step').forEach(el => el.classList.remove('active'));");
    client.println("  document.getElementById('step' + step).classList.add('active');");
    client.println("  currentStep = step;");
    client.println("}");

    client.println("document.addEventListener('DOMContentLoaded', function() {");
    client.println("  initializeChart();");
    client.println("});");

    client.println("</script>");
    client.println("</body></html>");
}

String WebServer::generateCalibrationWizardHTML() {
    String html = "<div class='container'>";
    html += "<h1>Sensor Calibration Wizard</h1>";
    html += "<p><a href='/'>← Back to Home</a></p>";

    // Step 1: Sensor Selection
    html += "<div id='step1' class='wizard-step active card'>";
    html += "<h2>Step 1: Select Sensor to Calibrate</h2>";
    html += "<button class='button' onclick='startCalibration(\"ultrasonic1\")'>Calibrate Front Ultrasonic</button>";
    html += "<button class='button' onclick='startCalibration(\"ultrasonic2\")'>Calibrate Rear Ultrasonic</button>";
    html += "<button class='button' onclick='startCalibration(\"pressure\")'>Calibrate Pressure Sensor</button>";
    html += "</div>";

    // Step 2: Calibration in Progress
    html += "<div id='step2' class='wizard-step card'>";
    html += "<h2>Step 2: Calibration in Progress</h2>";
    html += "<p>Collecting sensor readings for baseline calculation...</p>";
    html += "<div class='progress-bar'><div id='progressFill' class='progress-fill' style='width: 0%'></div></div>";
    html += "<div class='sensor-reading'>";
    html += "Readings: <span id='readingCount'>0</span>/100<br>";
    html += "Min: <span id='minValue'>0</span> | Max: <span id='maxValue'>0</span> | Avg: <span id='avgValue'>0</span>";
    html += "</div>";
    html += "<canvas id='calibrationChart' width='400' height='200'></canvas>";
    html += "<button id='saveBtn' class='button' disabled onclick='saveCalibration()'>Save Calibration</button>";
    html += "<button class='button danger' onclick='fetch(\"/api/calibrate-reset\").then(() => location.reload())'>Cancel</button>";
    html += "</div>";

    // Step 3: Threshold Adjustment
    html += "<div id='step3' class='wizard-step card'>";
    html += "<h2>Step 3: Adjust Thresholds</h2>";
    html += "<p>Fine-tune detection thresholds based on your environment:</p>";
    html += "<label>Warning Distance (cm): <input type='range' class='threshold-slider' min='10' max='100' value='30' id='warningSlider'></label>";
    html += "<label>Critical Distance (cm): <input type='range' class='threshold-slider' min='5' max='50' value='10' id='criticalSlider'></label>";
    html += "<button class='button' onclick='showWizardStep(4)'>Continue</button>";
    html += "</div>";

    // Step 4: Completion
    html += "<div id='step4' class='wizard-step card'>";
    html += "<h2>Calibration Complete!</h2>";
    html += "<div class='calibration-status status-ok'>";
    html += "✓ Sensor calibration saved successfully<br>";
    html += "✓ Baseline values recorded<br>";
    html += "✓ Thresholds updated";
    html += "</div>";
    html += "<button class='button' onclick='exportCalibration()'>Export Settings</button>";
    html += "<button class='button' onclick='location.href=\"/\"'>Return to Dashboard</button>";
    html += "</div>";

    html += "</div>";
    return html;
}

void WebServer::sendJsonResponse(WiFiClient& client, String jsonData) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println();
    client.println(jsonData);
}

void WebServer::sendHttpHeader(WiFiClient& client, const char* contentType) {
    client.println("HTTP/1.1 200 OK");
    client.print("Content-Type: ");
    client.println(contentType);
    client.println("Connection: close");
    client.println();
}

void WebServer::sendHomePage(WiFiClient& client) {
    sendHttpHeader(client);

    client.println("<!DOCTYPE html>");
    client.println("<html lang='en'>");
    client.println("<head>");
    client.println("<meta charset='UTF-8'>");
    client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
    client.println("<title>Wheelchair Swing Control Center</title>");
    client.println("<link href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/css/all.min.css' rel='stylesheet'>");
    client.println("<style>");

    // Modern CSS styling
    client.println("* { margin: 0; padding: 0; box-sizing: border-box; }");
    client.println("body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh; }");
    client.println(".container { max-width: 1200px; margin: 0 auto; padding: 20px; }");
    client.println(".header { background: rgba(255,255,255,0.95); border-radius: 15px; padding: 30px; margin-bottom: 30px; backdrop-filter: blur(10px); box-shadow: 0 8px 32px rgba(0,0,0,0.1); }");
    client.println(".header h1 { color: #2d3748; font-size: 2.5rem; text-align: center; margin-bottom: 10px; }");
    client.println(".header .subtitle { text-align: center; color: #718096; font-size: 1.1rem; }");

    client.println(".grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; margin-bottom: 30px; }");
    client.println(".card { background: rgba(255,255,255,0.95); border-radius: 15px; padding: 25px; backdrop-filter: blur(10px); box-shadow: 0 8px 32px rgba(0,0,0,0.1); transition: transform 0.3s ease; }");
    client.println(".card:hover { transform: translateY(-5px); }");
    client.println(".card h3 { color: #2d3748; margin-bottom: 15px; font-size: 1.3rem; display: flex; align-items: center; }");
    client.println(".card h3 i { margin-right: 10px; color: #667eea; }");

    client.println(".status-item { display: flex; justify-content: space-between; align-items: center; padding: 12px 0; border-bottom: 1px solid #e2e8f0; }");
    client.println(".status-item:last-child { border-bottom: none; }");
    client.println(".status-label { font-weight: 600; color: #4a5568; }");
    client.println(".status-value { font-weight: 700; }");

    client.println(".status-ok { color: #38a169; }");
    client.println(".status-warning { color: #d69e2e; }");
    client.println(".status-error { color: #e53e3e; }");
    client.println(".status-emergency { color: #e53e3e; animation: blink 1s infinite; }");
    client.println("@keyframes blink { 0%, 50% { opacity: 1; } 51%, 100% { opacity: 0.5; } }");

    client.println(".nav-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; margin-bottom: 30px; }");
    client.println(".nav-button { display: flex; align-items: center; justify-content: center; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; padding: 18px; text-decoration: none; border-radius: 12px; font-weight: 600; transition: all 0.3s ease; box-shadow: 0 4px 15px rgba(102, 126, 234, 0.4); }");
    client.println(".nav-button:hover { transform: translateY(-2px); box-shadow: 0 8px 25px rgba(102, 126, 234, 0.6); }");
    client.println(".nav-button i { margin-right: 8px; font-size: 1.2rem; }");

    client.println(".control-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); gap: 15px; }");
    client.println(".control-button { display: flex; flex-direction: column; align-items: center; justify-content: center; background: #4299e1; color: white; padding: 20px; text-decoration: none; border-radius: 12px; font-weight: 600; transition: all 0.3s ease; min-height: 100px; box-shadow: 0 4px 15px rgba(66, 153, 225, 0.3); }");
    client.println(".control-button:hover { transform: translateY(-2px); box-shadow: 0 6px 20px rgba(66, 153, 225, 0.5); }");
    client.println(".control-button.success { background: #48bb78; box-shadow: 0 4px 15px rgba(72, 187, 120, 0.3); }");
    client.println(".control-button.warning { background: #ed8936; box-shadow: 0 4px 15px rgba(237, 137, 54, 0.3); }");
    client.println(".control-button.emergency { background: #f56565; box-shadow: 0 4px 15px rgba(245, 101, 101, 0.3); animation: pulse 2s infinite; }");
    client.println("@keyframes pulse { 0% { box-shadow: 0 4px 15px rgba(245, 101, 101, 0.3); } 50% { box-shadow: 0 8px 30px rgba(245, 101, 101, 0.6); } 100% { box-shadow: 0 4px 15px rgba(245, 101, 101, 0.3); } }");
    client.println(".control-button i { font-size: 2rem; margin-bottom: 8px; }");

    client.println(".footer { text-align: center; color: rgba(255,255,255,0.8); margin-top: 30px; padding: 20px; }");
    client.println("@media (max-width: 768px) { .header h1 { font-size: 2rem; } .grid { grid-template-columns: 1fr; } }");

    client.println("</style>");
    client.println("</head>");
    client.println("<body>");

    client.println("<div class='container'>");
    client.println("<div class='header'>");
    client.println("<h1><i class='fas fa-wheelchair'></i> Wheelchair Swing Control Center</h1>");
    client.println("<p class='subtitle'>Advanced Safety & Control System</p>");
    client.println("</div>");

    // System Status Card
    client.println("<div class='grid'>");
    client.println("<div class='card'>");
    client.println("<h3><i class='fas fa-tachometer-alt'></i>System Status</h3>");

    String statusClass = "status-ok";
    if (_safetyMonitor->getCurrentStatus() == SafetyMonitor::STATUS_WARNING) statusClass = "status-warning";
    else if (_safetyMonitor->getCurrentStatus() == SafetyMonitor::STATUS_ERROR) statusClass = "status-error";
    else if (_safetyMonitor->getCurrentStatus() == SafetyMonitor::STATUS_EMERGENCY) statusClass = "status-emergency";

    client.println("<div class='status-item'>");
    client.println("<span class='status-label'>State:</span>");
    client.print("<span class='status-value ");
    client.print(statusClass);
    client.print("'>");
    client.print(_stateMachine->getStateString());
    client.println("</span></div>");

    client.println("<div class='status-item'>");
    client.println("<span class='status-label'>Speed:</span>");
    client.print("<span class='status-value'>");
    client.print(_stateMachine->getSpeedString());
    client.println("</span></div>");

    client.println("<div class='status-item'>");
    client.println("<span class='status-label'>Safety:</span>");
    client.print("<span class='status-value ");
    client.print(statusClass);
    client.print("'>");
    client.print(_safetyMonitor->getStatusString());
    client.println("</span></div>");

    client.println("</div>");

    // Sensor Data Card
    client.println("<div class='card'>");
    client.println("<h3><i class='fas fa-radar'></i>Sensor Readings</h3>");

    client.println("<div class='status-item'>");
    client.println("<span class='status-label'>Front Distance:</span>");
    client.print("<span class='status-value'>");
    client.print(_safetyMonitor->getFrontDistance(), 1);
    client.println(" cm</span></div>");

    client.println("<div class='status-item'>");
    client.println("<span class='status-label'>Rear Distance:</span>");
    client.print("<span class='status-value'>");
    client.print(_safetyMonitor->getRearDistance(), 1);
    client.println(" cm</span></div>");

    client.println("<div class='status-item'>");
    client.println("<span class='status-label'>User Present:</span>");
    client.print("<span class='status-value ");
    client.print(_safetyMonitor->isUserPresent() ? "status-ok" : "status-warning");
    client.print("'>");
    client.print(_safetyMonitor->isUserPresent() ? "Yes" : "No");
    client.println("</span></div>");

    client.println("</div>");
    client.println("</div>");

    // Navigation
    client.println("<div class='nav-grid'>");
    client.println("<a href='/control' class='nav-button'><i class='fas fa-gamepad'></i>Control Panel</a>");
    client.println("<a href='/status' class='nav-button'><i class='fas fa-chart-line'></i>Status Monitor</a>");
    client.println("<a href='/config' class='nav-button'><i class='fas fa-cog'></i>Configuration</a>");
    client.println("<a href='/debug' class='nav-button'><i class='fas fa-bug'></i>Debug Tools</a>");
    client.println("</div>");

    // Quick Controls
    client.println("<div class='card'>");
    client.println("<h3><i class='fas fa-bolt'></i>Quick Actions</h3>");
    client.println("<div class='control-grid'>");

    if (_stateMachine->getCurrentState() == StateMachine::STATE_EMERGENCY) {
        client.println("<a href='/api/reset' class='control-button emergency'><i class='fas fa-exclamation-triangle'></i>EMERGENCY RESET</a>");
    } else {
        client.println("<a href='/api/start' class='control-button success'><i class='fas fa-play'></i>Start</a>");
        client.println("<a href='/api/stop' class='control-button warning'><i class='fas fa-stop'></i>Stop</a>");
        client.println("<a href='/api/emergency' class='control-button emergency'><i class='fas fa-exclamation-triangle'></i>Emergency</a>");
    }

    client.println("</div>");
    client.println("</div>");

    client.println("<div class='footer'>");
    client.println("<p>Wheelchair Swing Control System v1.0 | Auto-refresh in <span id='countdown'>10</span>s</p>");
    client.println("</div>");

    client.println("</div>");

    // Auto-refresh script
    client.println("<script>");
    client.println("let countdown = 10;");
    client.println("setInterval(() => {");
    client.println("  countdown--;");
    client.println("  document.getElementById('countdown').textContent = countdown;");
    client.println("  if (countdown <= 0) location.reload();");
    client.println("}, 1000);");
    client.println("</script>");

    client.println("</body></html>");
}

void WebServer::sendControlPage(WiFiClient& client) {
    sendHttpHeader(client);

    client.println("<!DOCTYPE html>");
    client.println("<html lang='en'>");
    client.println("<head>");
    client.println("<meta charset='UTF-8'>");
    client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
    client.println("<title>Control Panel - Wheelchair Swing</title>");
    client.println("<link href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/css/all.min.css' rel='stylesheet'>");

    // Include the same modern CSS as home page
    client.println("<style>");
    client.println("* { margin: 0; padding: 0; box-sizing: border-box; }");
    client.println("body { font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); min-height: 100vh; }");
    client.println(".container { max-width: 800px; margin: 0 auto; padding: 20px; }");
    client.println(".header { background: rgba(255,255,255,0.95); border-radius: 15px; padding: 30px; margin-bottom: 30px; backdrop-filter: blur(10px); box-shadow: 0 8px 32px rgba(0,0,0,0.1); text-align: center; }");
    client.println(".control-section { background: rgba(255,255,255,0.95); border-radius: 15px; padding: 25px; margin-bottom: 20px; backdrop-filter: blur(10px); box-shadow: 0 8px 32px rgba(0,0,0,0.1); }");
    client.println(".control-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; }");
    client.println(".control-button { display: flex; flex-direction: column; align-items: center; justify-content: center; color: white; padding: 25px; text-decoration: none; border-radius: 12px; font-weight: 600; transition: all 0.3s ease; min-height: 120px; }");
    client.println(".control-button:hover { transform: translateY(-3px); }");
    client.println(".control-button i { font-size: 2.5rem; margin-bottom: 10px; }");
    client.println(".success { background: linear-gradient(135deg, #48bb78, #38a169); }");
    client.println(".primary { background: linear-gradient(135deg, #4299e1, #3182ce); }");
    client.println(".warning { background: linear-gradient(135deg, #ed8936, #dd6b20); }");
    client.println(".emergency { background: linear-gradient(135deg, #f56565, #e53e3e); animation: pulse 2s infinite; }");
    client.println(".back-button { display: inline-block; background: #718096; color: white; padding: 12px 24px; text-decoration: none; border-radius: 8px; margin-bottom: 20px; }");
    client.println("</style>");

    client.println("</head>");
    client.println("<body>");
    client.println("<div class='container'>");

    client.println("<div class='header'>");
    client.println("<h1><i class='fas fa-gamepad'></i> Control Panel</h1>");
    client.println("<a href='/' class='back-button'><i class='fas fa-arrow-left'></i> Back to Home</a>");
    client.println("</div>");

    // Basic Controls
    client.println("<div class='control-section'>");
    client.println("<h3><i class='fas fa-play-circle'></i> Basic Controls</h3>");
    client.println("<div class='control-grid'>");
    client.println("<a href='/api/start' class='control-button success'><i class='fas fa-play'></i>START SWING</a>");
    client.println("<a href='/api/stop' class='control-button warning'><i class='fas fa-stop'></i>STOP SWING</a>");
    client.println("</div>");
    client.println("</div>");

    // Speed Controls
    client.println("<div class='control-section'>");
    client.println("<h3><i class='fas fa-tachometer-alt'></i> Speed Controls</h3>");
    client.println("<div class='control-grid'>");
    client.println("<a href='/api/speed-up' class='control-button primary'><i class='fas fa-arrow-up'></i>INCREASE SPEED</a>");
    client.println("<a href='/api/speed-down' class='control-button primary'><i class='fas fa-arrow-down'></i>DECREASE SPEED</a>");
    client.println("</div>");
    client.println("</div>");

    // Door Controls
    client.println("<div class='control-section'>");
    client.println("<h3><i class='fas fa-door-open'></i> Door Controls</h3>");
    client.println("<div class='control-grid'>");
    client.println("<a href='/api/door-open' class='control-button primary'><i class='fas fa-door-open'></i>OPEN DOOR</a>");
    client.println("<a href='/api/door-close' class='control-button primary'><i class='fas fa-door-closed'></i>CLOSE DOOR</a>");
    client.println("</div>");
    client.println("</div>");

    // Safety Controls
    client.println("<div class='control-section'>");
    client.println("<h3><i class='fas fa-shield-alt'></i> Safety Controls</h3>");
    client.println("<div class='control-grid'>");
    client.println("<a href='/api/emergency' class='control-button emergency'><i class='fas fa-exclamation-triangle'></i>EMERGENCY STOP</a>");

    if (_stateMachine->getCurrentState() == StateMachine::STATE_EMERGENCY) {
        client.println("<a href='/api/reset' class='control-button success'><i class='fas fa-undo'></i>RESET FROM EMERGENCY</a>");
    }
    if (_stateMachine->getCurrentState() == StateMachine::STATE_ERROR) {
        client.println("<a href='/api/clear-error' class='control-button success'><i class='fas fa-check'></i>CLEAR ERROR</a>");
    }

    client.println("</div>");
    client.println("</div>");

    client.println("</div>");
    client.println("</body></html>");
}

void WebServer::sendStatusPage(WiFiClient& client) {
    sendHttpHeader(client);

    client.println("<!DOCTYPE html>");
    client.println("<html><head><title>Status Monitor</title>");
    client.println("<meta name='viewport' content='width=device-width, initial-scale=1'>");
    client.println("<meta http-equiv='refresh' content='2'>");
    client.println("</head><body>");
    client.println("<h1>Status Monitor</h1>");
    client.println("<p><a href='/'>← Back to Home</a></p>");
    client.println("<p>Real-time status monitoring page (auto-refreshes every 2 seconds)</p>");
    client.println("</body></html>");
}

void WebServer::sendConfigPage(WiFiClient& client) {
    sendHttpHeader(client);

    client.println("<!DOCTYPE html>");
    client.println("<html><head><title>Configuration</title>");
    client.println("<meta name='viewport' content='width=device-width, initial-scale=1'>");
    client.println("</head><body>");
    client.println("<h1>Configuration</h1>");
    client.println("<p><a href='/'>← Back to Home</a></p>");
    client.println("<p>Configuration settings coming soon...</p>");
    client.println("</body></html>");
}

void WebServer::sendDebugPage(WiFiClient& client) {
    sendHttpHeader(client);

    client.println("<!DOCTYPE html>");
    client.println("<html><head><title>Debug Tools</title>");
    client.println("<meta name='viewport' content='width=device-width, initial-scale=1'>");
    client.println("</head><body>");
    client.println("<h1>Debug Tools</h1>");
    client.println("<p><a href='/'>← Back to Home</a></p>");
    client.println("<p>Debug information and tools coming soon...</p>");
    client.println("</body></html>");
}

void WebServer::send404Page(WiFiClient& client) {
    client.println("HTTP/1.1 404 Not Found");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.println();
    client.println("<!DOCTYPE html>");
    client.println("<html><head><title>404 Not Found</title></head><body>");
    client.println("<h1>404 - Page Not Found</h1>");
    client.println("<p><a href='/'>← Back to Home</a></p>");
    client.println("</body></html>");
}

void WebServer::handleControlCommand(WiFiClient& client, String command) {
    Serial.print("API Command received: ");
    Serial.println(command);

    if (command == "start") {
        _stateMachine->processEvent(StateMachine::EVENT_START_PRESSED);
    } else if (command == "stop") {
        _stateMachine->processEvent(StateMachine::EVENT_STOP_PRESSED);
    } else if (command == "speed-up") {
        _stateMachine->processEvent(StateMachine::EVENT_SPEED_UP);
    } else if (command == "speed-down") {
        _stateMachine->processEvent(StateMachine::EVENT_SPEED_DOWN);
    } else if (command == "door-open" || command == "door-close") {
        _stateMachine->processEvent(StateMachine::EVENT_DOOR_TOGGLE);
    } else if (command == "emergency") {
        _stateMachine->processEvent(StateMachine::EVENT_EMERGENCY);
    } else if (command == "reset") {
        _stateMachine->processEvent(StateMachine::EVENT_EMERGENCY_RESET);
    } else if (command == "clear-error") {
        _stateMachine->processEvent(StateMachine::EVENT_ERROR_CLEARED);
    }

    // Send response and redirect back
    client.println("HTTP/1.1 302 Found");
    client.println("Location: /");
    client.println("Connection: close");
    client.println();
}

void WebServer::handleConfigUpdate(WiFiClient& client, String params) {
    RuntimeConfig& config = RuntimeConfig::getInstance();

    // Parse: setting=frontWarningDistance&value=35.0
    int settingStart = params.indexOf("setting=") + 8;
    int settingEnd = params.indexOf("&", settingStart);
    String setting = params.substring(settingStart, settingEnd);

    int valueStart = params.indexOf("value=") + 6;
    String value = params.substring(valueStart);

    bool success = false;

    if (setting == "frontWarningDistance") {
        success = config.setWarningDistance(value.toFloat());
    } else if (setting == "frontCriticalDistance") {
        success = config.setCriticalDistance(value.toFloat());
    } else if (setting == "pressureThreshold") {
        success = config.setPressureThreshold(value.toInt());
    } else if (setting == "speedLow") {
        success = config.setSpeedLow(value.toInt());
    } else if (setting == "speedMedium") {
        success = config.setSpeedMedium(value.toInt());
    } else if (setting == "speedHigh") {
        success = config.setSpeedHigh(value.toInt());
    } else if (setting == "audioFeedback") {
        config.setAudioFeedbackEnabled(value == "true");
        success = true;
    } else if (setting == "doorTimeout") {
        success = config.setDoorTimeoutMs(value.toInt());
    }

    if (success) {
        config.save(); // Save immediately
        sendJsonResponse(client, "{\"status\":\"success\",\"message\":\"Setting updated and saved\"}");
    } else {
        sendJsonResponse(client, "{\"error\":\"Invalid setting or value out of range\"}");
    }
}

void WebServer::exportConfiguration(WiFiClient& client) {
    RuntimeConfig& config = RuntimeConfig::getInstance();
    String jsonConfig = config.exportToJson();

    sendHttpHeader(client, "application/json");
    client.println(jsonConfig);
}

String WebServer::getSystemStatus() {
    return String(_stateMachine->getStateString());
}

String WebServer::getCurrentDateTime() {
    return String(millis() / 1000);
}
