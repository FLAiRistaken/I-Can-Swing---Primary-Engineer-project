// lib/Connectivity/WebServer.cpp
#include "WebServer.h"
#include <Arduino.h>

WebServer::WebServer(StateMachine* stateMachine, SafetyMonitor* safetyMonitor)
    : _server(80), _stateMachine(stateMachine), _safetyMonitor(safetyMonitor), _logIndex(0) {
    // Initialize logs
    for (uint8_t i = 0; i < MAX_LOGS; i++) {
        _logs[i] = {0, "", ""};
    }
}

void WebServer::begin(int port) {
    _server.begin();
    Serial.print("WebServer: Started on port ");
    Serial.println(port);
    logEvent("Server", "Started");
}

void WebServer::handleClient() {
    WiFiClient client = _server.available();
    if (!client) return;

    // Wait for data with timeout
    unsigned long timeout = millis() + 1000;
    while (!client.available() && millis() < timeout) {
        delay(1);
    }

    if (!client.available()) {
        client.stop();
        return;
    }

    // Read request line only
    String request = client.readStringUntil('\n');

    // Clear remaining data
    while (client.available()) {
        client.read();
    }

    Serial.print("WebServer: ");
    Serial.println(request.substring(0, 50));

    bool isAjaxRequest = (request.indexOf("/api/") >= 0);

    // Route requests
    if (request.indexOf("GET / ") >= 0) {
        sendHomePage(client);
    } else if (request.indexOf("GET /control") >= 0) {
        sendControlPage(client);
    } else if (request.indexOf("GET /config") >= 0) {
        sendConfigPage(client);
    } else if (request.indexOf("GET /api/control?") >= 0) {
        int start = request.indexOf("cmd=") + 4;
        int end = request.indexOf(" ", start);
        String command = request.substring(start, end);
        handleControlAPI(client, command);
    } else if (request.indexOf("GET /api/config?") >= 0) {
        int start = request.indexOf("?") + 1;
        int end = request.indexOf(" HTTP", start);
        String params = request.substring(start, end);
        handleConfigAPI(client, params);
    } else if (request.indexOf("GET /api/status") >= 0) {
        handleStatusAPI(client);
    } else {
        send404Page(client);
    }

    if (isAjaxRequest) {
        // Keep connection open briefly for potential follow-up requests
        delay(10);  // Small delay to allow browser to reuse connection
    }

    client.stop();
}

// ===== PAGE HANDLERS =====

void WebServer::sendHomePage(WiFiClient& client) {
    unsigned long startTime = millis();

    sendPageHeader(client, "I Can Swing - Home");

    // Build entire home page efficiently
    String homePage;
    homePage.reserve(3072);

    homePage += "<h2>System Status</h2>";

    // System state overview
    homePage += "<div class=\"status-grid\">";
    homePage += "<div class=\"status-item\"><strong>State:</strong><br>" + String(_stateMachine->getStateString()) + "</div>";
    homePage += "<div class=\"status-item\"><strong>Speed:</strong><br>" + String(_stateMachine->getSpeedString()) + "</div>";

    // Safety status with proper method
    SafetyMonitor::SafetyStatus safetyStatus = _safetyMonitor->getCurrentStatus();
    homePage += "<div class=\"status-item\"><strong>Safety:</strong><br>";
    switch(safetyStatus) {
        case SafetyMonitor::STATUS_OK:
            homePage += "<span style=\"color: green\">✅ All Clear</span>";
            break;
        case SafetyMonitor::STATUS_WARNING:
            homePage += "<span style=\"color: orange\">⚠️ Warning</span>";
            break;
        case SafetyMonitor::STATUS_ERROR:
            homePage += "<span style=\"color: red\">❌ Error</span>";
            break;
        case SafetyMonitor::STATUS_EMERGENCY:
            homePage += "<span style=\"color: red; font-weight: bold\">🚨 Emergency</span>";
            break;
        default:
            homePage += "<span style=\"color: gray\">❓ Unknown</span>";
    }
    homePage += "</div>";

    // System uptime
    unsigned long seconds = millis() / 1000;
    unsigned long hours = seconds / 3600;
    unsigned long minutes = (seconds % 3600) / 60;
    seconds = seconds % 60;
    homePage += "<div class=\"status-item\"><strong>Uptime:</strong><br>" + String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s</div>";
    homePage += "</div>";

    client.print(homePage);  // Send first chunk

    // Sensor data section
    String sensorData = getSensorData();
    client.print(sensorData);

    // Quick actions section
    String quickActions;
    quickActions.reserve(1024);
    quickActions += "<h2>Quick Actions</h2>";
    quickActions += "<div class=\"button-grid\">";
    quickActions += "<a href=\"/control\" class=\"btn btn-primary btn-large\">🎮 Control Panel</a>";
    quickActions += "<a href=\"/config\" class=\"btn btn-secondary btn-large\">⚙️ Configuration</a>";
    quickActions += "<button onclick=\"location.reload()\" class=\"btn btn-info\">🔄 Refresh</button>";

    // Context-sensitive quick controls
    if (_stateMachine->getCurrentState() == StateMachine::STATE_IDLE) {
        quickActions += "<a href=\"/api/control?cmd=speed_low\" class=\"btn btn-success\">🚀 Start Low Speed</a>";
    } else if (_stateMachine->getCurrentState() == StateMachine::STATE_SWINGING) {
        quickActions += "<a href=\"/api/control?cmd=stop\" class=\"btn btn-warning\">⏹️ Stop Swing</a>";
    }
    quickActions += "<a href=\"/api/control?cmd=emergency\" class=\"btn btn-danger\">🚨 Emergency</a>";
    quickActions += "</div>";

    client.print(quickActions);

    // Recent activity logs
    String recentLogs = getRecentLogsOptimized();
    client.print(recentLogs);

    // Auto-refresh (less aggressive than before)
    client.print("<script>setTimeout(() => location.reload(), 30000);</script>");  // 30 second refresh
    client.print("</main></body></html>");

    logResponseTime(startTime);
}


void WebServer::sendControlPage(WiFiClient& client) {
    unsigned long startTime = millis();

    sendPageHeader(client, "Swing Control");

    // Build control page efficiently with AJAX buttons
    String controlPage;
    controlPage.reserve(2048);

    controlPage += "<h2>Swing Control Panel</h2>";

    // Current Status Section (will be updated via AJAX)
    controlPage += "<div class=\"status-grid\">";
    controlPage += "<div class=\"status-item\"><strong>System State:</strong><br>" + String(_stateMachine->getStateString()) + "</div>";
    controlPage += "<div class=\"status-item\"><strong>Current Speed:</strong><br>" + String(_stateMachine->getSpeedString()) + "</div>";

    // Safety status
    SafetyMonitor::SafetyStatus safetyStatus = _safetyMonitor->getCurrentStatus();
    controlPage += "<div class=\"status-item\"><strong>Safety Status:</strong><br>";
    switch(safetyStatus) {
        case SafetyMonitor::STATUS_OK:
            controlPage += "<span style=\"color: green\">✅ All Clear</span>";
            break;
        case SafetyMonitor::STATUS_WARNING:
            controlPage += "<span style=\"color: orange\">⚠️ Warning</span>";
            break;
        case SafetyMonitor::STATUS_ERROR:
            controlPage += "<span style=\"color: red\">❌ Error</span>";
            break;
        case SafetyMonitor::STATUS_EMERGENCY:
            controlPage += "<span style=\"color: red; font-weight: bold\">🚨 Emergency</span>";
            break;
        default:
            controlPage += "<span style=\"color: gray\">❓ Unknown</span>";
    }
    controlPage += "</div>";

    unsigned long seconds = millis() / 1000;
    controlPage += "<div class=\"status-item\"><strong>Uptime:</strong><br>" + String(seconds) + " seconds</div>";
    controlPage += "</div>";

    // AJAX-enabled Main Control Buttons
    controlPage += "<div class=\"control-section\">";
    controlPage += "<h3>🎢 Swing Controls</h3>";
    controlPage += "<div class=\"button-grid\">";

    // Show different buttons based on current state
    StateMachine::State currentState = _stateMachine->getCurrentState();

    if (currentState == StateMachine::STATE_EMERGENCY) {
        // Emergency state
        controlPage += "<button onclick=\"cmd('reset')\" class=\"btn btn-warning btn-large\">🔄 RESET FROM EMERGENCY</button>";
        controlPage += "<button onclick=\"cmd('emergency')\" class=\"btn btn-danger\">🚨 EMERGENCY (again)</button>";
    } else {
        // Normal operation - show regular controls
        controlPage += "<button onclick=\"cmd('speed_low')\" class=\"btn btn-success btn-large\">🐌 Low Speed</button>";
        controlPage += "<button onclick=\"cmd('speed_medium')\" class=\"btn btn-warning btn-large\">🚶 Medium Speed</button>";
        controlPage += "<button onclick=\"cmd('speed_high')\" class=\"btn btn-info btn-large\">🏃 High Speed</button>";
        controlPage += "<button onclick=\"cmd('stop')\" class=\"btn btn-primary btn-large\">⏹️ Stop</button>";
        controlPage += "<button onclick=\"cmd('emergency')\" class=\"btn btn-danger btn-large\">🚨 EMERGENCY</button>";

        // Show reset button smaller when not in emergency
        if (currentState == StateMachine::STATE_ERROR) {
            controlPage += "<button onclick=\"cmd('reset')\" class=\"btn btn-secondary\">🔄 Reset</button>";
        }
    }

controlPage += "</div></div>";

    // AJAX-enabled Door Controls
    controlPage += "<div class=\"control-section\">";
    controlPage += "<h3>🚪 Door Controls</h3>";
    controlPage += "<div class=\"button-grid\">";
    controlPage += "<button onclick=\"cmd('door_open')\" class=\"btn btn-success\">🔓 Open Doors</button>";
    controlPage += "<button onclick=\"cmd('door_close')\" class=\"btn btn-secondary\">🔒 Close Doors</button>";
    controlPage += "</div></div>";

    client.print(controlPage);

    // Live physics data section
    String liveData;
    liveData.reserve(512);
    liveData += "<div class=\"control-section\">";
    liveData += "<h3>📊 Live Physics Data</h3>";
    liveData += "<div class=\"physics-monitor\">";

    RuntimeConfig& config = RuntimeConfig::getInstance();
    liveData += "<div class=\"physics-value\">Push: " + String(config.getPushDurationPercent()) + "% duration, ";
    liveData += String(config.getPushPowerPercent()) + "% power</div>";
    liveData += "<div class=\"physics-value\">Period: " + String(config.getSwingPeriodMs()) + "ms, ";
    liveData += "Max: " + String(config.getSwingMaxAngleDegrees()) + "°</div>";
    liveData += "</div>";
    liveData += "<button onclick=\"location.reload()\" class=\"btn btn-info\">🔄 Full Refresh</button>";
    liveData += "</div>";

    client.print(liveData);

    // Add the optimized AJAX script
    String ajaxScript = getOptimizedAjaxScript();
    client.print(ajaxScript);

    client.print("</main></body></html>");

    logResponseTime(startTime);
}



// Add this optimized method to WebServer.cpp
void WebServer::sendPhysicsConfigSection(WiFiClient& client) {
    RuntimeConfig& config = RuntimeConfig::getInstance();
    String physics;
    physics.reserve(2048);  // Pre-allocate for performance

    physics += "<div class=\"config-section\">";
    physics += "<h3>🎢 Pendulum Physics Settings</h3>";

    physics += "<label>Push Duration (%): <input type=\"number\" name=\"pushDuration\" value=\"";
    physics += String(config.getPushDurationPercent());
    physics += "\" min=\"10\" max=\"50\" step=\"1\"></label>";
    physics += "<div class=\"physics-help\">Push Duration: Percentage of swing cycle to apply power (20% = natural, 30% = more aggressive)</div>";

    physics += "<label>Push Power (%): <input type=\"number\" name=\"pushPower\" value=\"";
    physics += String(config.getPushPowerPercent());
    physics += "\" min=\"50\" max=\"100\" step=\"5\"></label>";
    physics += "<div class=\"physics-help\">Push Power: Motor power during push phase (100% = maximum torque, 80% = gentler motion)</div>";

    physics += "<label>Swing Steps - Low Speed: <input type=\"number\" name=\"swingStepsLow\" value=\"";
    physics += String(config.getSwingSpeedLowSteps());
    physics += "\" min=\"1\" max=\"255\" step=\"5\"></label>";

    physics += "<label>Swing Steps - Medium Speed: <input type=\"number\" name=\"swingStepsMed\" value=\"";
    physics += String(config.getSwingSpeedMediumSteps());
    physics += "\" min=\"1\" max=\"255\" step=\"5\"></label>";

    physics += "<label>Swing Steps - High Speed: <input type=\"number\" name=\"swingStepsHigh\" value=\"";
    physics += String(config.getSwingSpeedHighSteps());
    physics += "\" min=\"1\" max=\"255\" step=\"5\"></label>";
    physics += "<div class=\"physics-help\">Swing Steps: Number of motor steps per interval (higher = more powerful swinging)</div>";

    physics += "<label>Step Interval (ms): <input type=\"number\" name=\"stepInterval\" value=\"";
    physics += String(config.getSwingStepIntervalMs());
    physics += "\" min=\"10\" max=\"50\" step=\"1\"></label>";
    physics += "<div class=\"physics-help\">Step Interval: Time between motor updates in milliseconds (lower = smoother motion)</div>";

    physics += "</div>";

    client.print(physics);  // Single efficient send
}

// Add this optimized live monitor method
void WebServer::sendLiveMonitorSection(WiFiClient& client) {
    RuntimeConfig& config = RuntimeConfig::getInstance();
    String monitor;
    monitor.reserve(1024);

    monitor += "<div class=\"config-section\">";
    monitor += "<h3>📊 Live Physics Monitor</h3>";
    monitor += "<div id=\"physicsData\"><div class=\"physics-monitor\">";

    // Current settings display
    monitor += "<div class=\"physics-value\"><strong>Current Settings:</strong><br>";
    monitor += "Push Duration: " + String(config.getPushDurationPercent()) + "%<br>";
    monitor += "Push Power: " + String(config.getPushPowerPercent()) + "%<br>";
    monitor += "Swing Period: " + String(config.getSwingPeriodMs()) + "ms<br>";
    monitor += "Max Angle: " + String(config.getSwingMaxAngleDegrees()) + "°</div>";

    // System status display
    monitor += "<div class=\"physics-value\"><strong>System Status:</strong><br>";
    monitor += "State: " + String(_stateMachine->getStateString()) + "<br>";
    monitor += "Speed: " + String(_stateMachine->getSpeedString()) + "<br>";
    monitor += "Uptime: " + String(millis()/1000) + "s</div>";

    monitor += "</div></div>";

    // Manual refresh only (no auto-refresh for performance)
    monitor += "<button onclick=\"location.reload()\">🔄 Refresh Data</button>";
    monitor += "</div>";

    client.print(monitor);
}



void WebServer::sendConfigPage(WiFiClient& client) {
    unsigned long startTime = millis();

    sendPageHeader(client, "Configuration");

    RuntimeConfig& config = RuntimeConfig::getInstance();

    String configPage;
    configPage.reserve(4096);

    configPage += "<h2>System Configuration</h2>";
    configPage += "<form method='GET' action='/api/config'>";

    // Safety Settings Section - USING THE GENERIC SETTERS
    configPage += "<div class=\"config-section\">";
    configPage += "<h3>⚠️ Safety Settings</h3>";
    configPage += "<div class=\"physics-help\">These distances apply to both frontLeft and frontRight sensors</div>";

    configPage += "<label>Warning Distance (cm): <input type=\"number\" name=\"warningDistance\" value=\"";
    configPage += String(config.getFrontLeftWarningDistance()); // Same as frontRight, so use either
    configPage += "\" min=\"10\" max=\"200\" step=\"1\"></label>";
    configPage += "<div class=\"physics-help\">Distance at which the system gives a warning but continues operation</div>";

    configPage += "<label>Critical Distance (cm): <input type=\"number\" name=\"criticalDistance\" value=\"";
    configPage += String(config.getFrontLeftCriticalDistance()); // Same as frontRight, so use either
    configPage += "\" min=\"5\" max=\"100\" step=\"1\"></label>";
    configPage += "<div class=\"physics-help\">Distance at which the system immediately stops for safety</div>";
    configPage += "</div>";

    // Motor Speed Settings Section
    configPage += "<div class=\"config-section\">";
    configPage += "<h3>⚙️ Motor Speed Settings</h3>";
    configPage += "<label>Low Speed (RPM): <input type=\"number\" name=\"speedLow\" value=\"";
    configPage += String(config.getSpeedLow());
    configPage += "\" min=\"20\" max=\"50\" step=\"5\"></label>";
    configPage += "<label>Medium Speed (RPM): <input type=\"number\" name=\"speedMed\" value=\"";
    configPage += String(config.getSpeedMedium());
    configPage += "\" min=\"55\" max=\"70\" step=\"5\"></label>";
    configPage += "<label>High Speed (RPM): <input type=\"number\" name=\"speedHigh\" value=\"";
    configPage += String(config.getSpeedHigh());
    configPage += "\" min=\"75\" max=\"90\" step=\"5\"></label>";
    configPage += "</div>";

    // Basic Swing Settings Section
    configPage += "<div class=\"config-section\">";
    configPage += "<h3>🎢 Basic Swing Settings</h3>";
    configPage += "<label>Swing Period (ms): <input type=\"number\" name=\"swingPeriod\" value=\"";
    configPage += String(config.getSwingPeriodMs());
    configPage += "\" min=\"1000\" max=\"12000\" step=\"100\"></label>";
    configPage += "<div class=\"physics-help\">Total time for one complete swing cycle (lower = faster swinging)</div>";

    configPage += "<label>Max Swing Angle (degrees): <input type=\"number\" name=\"maxAngle\" value=\"";
    configPage += String(config.getSwingMaxAngleDegrees());
    configPage += "\" min=\"30\" max=\"180\" step=\"5\"></label>";
    configPage += "<div class=\"physics-help\">Maximum swing angle from center position (safety limit)</div>";

    configPage += "<label>Smooth Stop Time (ms): <input type=\"number\" name=\"smoothStop\" value=\"";
    configPage += String(config.getSwingSmoothStopMs());
    configPage += "\" min=\"1000\" max=\"10000\" step=\"500\"></label>";
    configPage += "<div class=\"physics-help\">Time to gradually stop the swing (longer = gentler stop)</div>";
    configPage += "</div>";

    client.print(configPage);

    // Send physics section (using our optimized method)
    sendPhysicsConfigSection(client);

    // Submit button and form close
    String formEnd;
    formEnd.reserve(256);
    formEnd += "<div class=\"config-section\">";
    formEnd += "<button type=\"submit\" class=\"btn btn-primary btn-large\">💾 Save All Settings</button>";
    formEnd += "<button type=\"button\" class=\"btn btn-secondary\" onclick=\"location.reload()\">🔄 Reset Form</button>";
    formEnd += "</div></form>";

    client.print(formEnd);

    // Send live monitor section
    sendLiveMonitorSection(client);

    client.print("</main></body></html>");

    logResponseTime(startTime);
}


void WebServer::send404Page(WiFiClient& client) {
    sendHttpHeader(client);
    client.println("<html><head><title>404 Not Found</title></head>");
    client.println("<body><h1>404 - Page Not Found</h1>");
    client.println("<a href='/'>Return to Home</a></body></html>");
}

// ===== API HANDLERS =====

// Keep handleControlAPI() minimal for fast AJAX responses
void WebServer::handleControlAPI(WiFiClient& client, String command) {
    unsigned long startTime = millis();

    bool success = false;
    String message = "";

    // Process commands quickly
    if (command == "speed_low") {
        _stateMachine->processEvent(StateMachine::EVENT_SPEED_SET_LOW);
        success = true;
        message = "Low speed set";
    }
    else if (command == "speed_medium") {
        _stateMachine->processEvent(StateMachine::EVENT_SPEED_SET_MEDIUM);
        success = true;
        message = "Medium speed set";
    }
    else if (command == "speed_high") {
        _stateMachine->processEvent(StateMachine::EVENT_SPEED_SET_HIGH);
        success = true;
        message = "High speed set";
    }
    else if (command == "stop") {
        _stateMachine->processEvent(StateMachine::EVENT_STOP_PRESSED);
        success = true;
        message = "Stopped";
    }
    else if (command == "emergency") {
        _stateMachine->processEvent(StateMachine::EVENT_EMERGENCY);
        success = true;
        message = "Emergency stop activated";
    }
    else if (command == "reset") {
        // Use both the event and direct method for safety
        _stateMachine->processEvent(StateMachine::EVENT_EMERGENCY_RESET);
        _stateMachine->resetFromEmergency();
        success = true;
        message = "System reset from emergency";
    }
    else if (command == "door_open") {
        _stateMachine->processEvent(StateMachine::EVENT_DOOR_OPEN_PRESSED);
        success = true;
        message = "Opening doors";
    }
    else if (command == "door_close") {
        _stateMachine->processEvent(StateMachine::EVENT_DOOR_CLOSE_PRESSED);
        success = true;
        message = "Closing doors";
    }
    else {
        message = "Unknown command";
    }

    // Ultra-compact JSON response for speed
    String response = "{\"success\":";
    response += (success ? "true" : "false");
    response += ",\"message\":\"";
    response += message;
    response += "\"}";

    sendJsonResponse(client, response);
    logResponseTime(startTime);
}


void WebServer::handleConfigAPI(WiFiClient& client, String params) {
    unsigned long startTime = millis();

    RuntimeConfig& config = RuntimeConfig::getInstance();
    bool success = true;
    String message = "Settings updated successfully";

    // Efficient parameter processing using helper method
    if (!processAllConfigParams(params, config)) {
        success = false;
        message = "One or more parameters were invalid";
    }

    // Auto-save to EEPROM if any changes were made
    if (success) {
        config.save();
        logEvent("Config", "Settings saved");
    }

    // Single JSON response
    String response;
    response.reserve(256);
    response += "{\"success\":";
    response += (success ? "true" : "false");
    response += ",\"message\":\"";
    response += message;
    response += "\",\"redirect\":\"/config\"}";

    sendJsonResponse(client, response);
    logResponseTime(startTime);
}

// Helper method for processing parameters efficiently
// Replace the existing processAllConfigParams method with this Arduino-compatible version:
bool WebServer::processAllConfigParams(String params, RuntimeConfig& config) {
    bool allSuccess = true;

    // Handle safety distances using the generic setters
    if (params.indexOf("warningDistance=") >= 0) {
        int start = params.indexOf("warningDistance=") + 16;
        int end = params.indexOf("&", start);
        if (end == -1) end = params.length();
        float value = params.substring(start, end).toFloat();
        if (!config.setWarningDistance(value)) {
            allSuccess = false;
            Serial.println("Failed to set warning distance");
        }
    }

    if (params.indexOf("criticalDistance=") >= 0) {
        int start = params.indexOf("criticalDistance=") + 17;
        int end = params.indexOf("&", start);
        if (end == -1) end = params.length();
        float value = params.substring(start, end).toFloat();
        if (!config.setCriticalDistance(value)) {
            allSuccess = false;
            Serial.println("Failed to set critical distance");
        }
    }

    // Handle integer parameters (more Arduino-compatible approach)
    struct IntParam {
        const char* name;
        int nameLen;
        bool (RuntimeConfig::*setter)(int);
    };

    IntParam intParams[] = {
        {"pushDuration=", 13, nullptr},  // We'll handle these manually since member function pointers are complex
        {"pushPower=", 10, nullptr},
        {"swingStepsLow=", 14, nullptr},
        {"swingStepsMed=", 14, nullptr},
        {"swingStepsHigh=", 15, nullptr},
        {"stepInterval=", 13, nullptr},
        {"swingPeriod=", 12, nullptr},
        {"maxAngle=", 9, nullptr},
        {"smoothStop=", 11, nullptr},
        {"speedLow=", 9, nullptr},
        {"speedMed=", 9, nullptr},
        {"speedHigh=", 10, nullptr}
    };

    // Manual handling for better Arduino compatibility
    for (int i = 0; i < 12; i++) {
        int pos = params.indexOf(intParams[i].name);
        if (pos >= 0) {
            int start = pos + intParams[i].nameLen;
            int end = params.indexOf("&", start);
            if (end == -1) end = params.length();
            int value = params.substring(start, end).toInt();

            // Handle each parameter explicitly
            bool result = false;
            switch(i) {
                case 0: result = config.setPushDurationPercent(value); break;
                case 1: result = config.setPushPowerPercent(value); break;
                case 2: result = config.setSwingSpeedLowSteps(value); break;
                case 3: result = config.setSwingSpeedMediumSteps(value); break;
                case 4: result = config.setSwingSpeedHighSteps(value); break;
                case 5: result = config.setSwingStepIntervalMs(value); break;
                case 6: result = config.setSwingPeriodMs(value); break;
                case 7: result = config.setSwingMaxAngleDegrees(value); break;
                case 8: result = config.setSwingSmoothStopMs(value); break;
                case 9: result = config.setSpeedLow(value); break;
                case 10: result = config.setSpeedMedium(value); break;
                case 11: result = config.setSpeedHigh(value); break;
            }

            if (!result) {
                allSuccess = false;
                Serial.print("Failed to set parameter: ");
                Serial.println(intParams[i].name);
            }
        }
    }

    return allSuccess;
}



// Enhanced handleStatusAPI() method for AJAX status updates
void WebServer::handleStatusAPI(WiFiClient& client) {
    unsigned long startTime = millis();

    // Create compact JSON status response
    String response;
    response.reserve(512);

    response += "{";
    response += "\"state\":\"" + String(_stateMachine->getStateString()) + "\",";
    response += "\"speed\":\"" + String(_stateMachine->getSpeedString()) + "\",";

    // Safety status
    SafetyMonitor::SafetyStatus safetyStatus = _safetyMonitor->getCurrentStatus();
    response += "\"safety\":\"";
    switch(safetyStatus) {
        case SafetyMonitor::STATUS_OK:
            response += "<span style='color:green'>✅ All Clear</span>";
            break;
        case SafetyMonitor::STATUS_WARNING:
            response += "<span style='color:orange'>⚠️ Warning</span>";
            break;
        case SafetyMonitor::STATUS_ERROR:
            response += "<span style='color:red'>❌ Error</span>";
            break;
        case SafetyMonitor::STATUS_EMERGENCY:
            response += "<span style='color:red;font-weight:bold'>🚨 Emergency</span>";
            break;
        default:
            response += "❓ Unknown";
    }
    response += "\",";

    response += "\"uptime\":" + String(millis() / 1000) + ",";

    // Add sensor data for more detailed status
    response += "\"frontLeftDistance\":" + String(_safetyMonitor->getFrontLeftDistance(), 1) + ",";
    response += "\"frontRightDistance\":" + String(_safetyMonitor->getFrontRightDistance(), 1) + ",";
    response += "\"userPresent\":" + String(_safetyMonitor->isUserPresent() ? "true" : "false");

    response += "}";

    sendJsonResponse(client, response);
    logResponseTime(startTime);
}


// ===== UTILITY METHODS =====

void WebServer::sendHttpHeader(WiFiClient& client, const char* contentType) {
    client.println("HTTP/1.1 200 OK");
    client.print("Content-Type: ");
    client.println(contentType);
    client.println("Connection: close");
    client.println();
}

void WebServer::sendJsonResponse(WiFiClient& client, const String& json) {
    sendHttpHeader(client, "application/json");
    client.println(json);
}

void WebServer::sendPageHeader(WiFiClient& client, const String& title) {
    String header;
    header.reserve(800);

    header += "HTTP/1.1 200 OK\r\n";
    header += "Content-Type: text/html; charset=UTF-8\r\n";  // ← ADD charset=UTF-8 here
    header += "Connection: close\r\n\r\n";
    header += "<!DOCTYPE html><html><head>";
    header += "<meta charset=\"UTF-8\">";  // ← ADD this meta tag for UTF-8
    header += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    header += "<title>" + title + "</title>";

    client.print(header);
    sendSimpleCSS(client);
    client.print("</head><body><header><h1>" + title + "</h1>");
    client.print("<nav><a href=\"/\">Home</a><a href=\"/control\">Control</a><a href=\"/config\">Configuration</a></nav></header><main>");
}


void WebServer::sendPageFooter(WiFiClient& client) {
    client.println("</main>");
    client.println("<footer>");
    client.print("<p>I Can Swing System - ");
    client.print(millis() / 1000);
    client.println(" seconds uptime</p>");
    client.println("</footer>");
    client.println("</body></html>");
}

void WebServer::sendSimpleCSS(WiFiClient& client) {
    String css;
    css.reserve(3000);  // Pre-allocate memory

    css += "<style>";
    css += "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background: #f5f5f5; }";
    css += "header { background: #2c3e50; color: white; padding: 15px; border-radius: 5px; margin-bottom: 20px; }";
    css += "header h1 { margin: 0; }";
    css += "nav { margin-top: 10px; }";
    css += "nav a { color: #ecf0f1; text-decoration: none; margin: 0 10px; }";
    css += "main { background: white; padding: 20px; border-radius: 5px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }";
    css += "h1, h2, h3 { color: #2c3e50; }";
    css += ".button-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); gap: 10px; margin: 15px 0; }";
    css += ".btn { padding: 12px 20px; border: none; border-radius: 5px; text-decoration: none; text-align: center; cursor: pointer; font-size: 14px; }";
    css += ".btn-large { padding: 20px; font-size: 18px; font-weight: bold; }";
    css += ".btn-primary { background: #3498db; color: white; }";
    css += ".btn-success { background: #27ae60; color: white; }";
    css += ".btn-warning { background: #f39c12; color: white; }";
    css += ".btn-danger { background: #e74c3c; color: white; }";
    css += ".btn-info { background: #17a2b8; color: white; }";
    css += ".btn-secondary { background: #6c757d; color: white; }";
    css += "button:hover { opacity: 0.8; }";
    css += ".status-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; margin: 15px 0; }";
    css += ".status-item { padding: 10px; background: #ecf0f1; border-radius: 5px; }";
    css += ".control-section { margin: 20px 0; padding: 15px; border: 1px solid #ddd; border-radius: 5px; }";
    css += ".config-section { background: white; padding: 20px; margin: 15px 0; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); border-left: 4px solid #3498db; }";
    css += ".config-section h3 { margin-top: 0; color: #2c3e50; border-bottom: 2px solid #ecf0f1; padding-bottom: 10px; }";
    css += "label { display: block; margin: 15px 0; font-weight: bold; color: #2c3e50; }";
    css += "input[type='number'] { width: 100px; padding: 8px; border: 1px solid #ddd; border-radius: 4px; font-size: 14px; }";
    css += "input[type='number']:focus { border-color: #3498db; outline: none; box-shadow: 0 0 5px rgba(52, 152, 219, 0.3); }";
    css += "input[type='checkbox'] { margin-right: 8px; }";
    css += ".physics-help { font-size: 0.85em; color: #7f8c8d; font-style: italic; margin: 5px 0 15px 0; padding: 8px; background: #f8f9fa; border-radius: 4px; border-left: 3px solid #17a2b8; }";
    css += "#physicsData { padding: 15px; background: #f8f9fa; border-radius: 4px; border: 1px solid #dee2e6; font-family: monospace; }";
    css += ".physics-monitor { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; }";
    css += ".physics-value { padding: 8px; background: white; border-radius: 4px; border-left: 3px solid #28a745; }";
    css += ".log-container { max-height: 200px; overflow-y: auto; border: 1px solid #ddd; padding: 10px; }";
    css += ".log-entry { padding: 5px; border-bottom: 1px solid #eee; }";
    css += "footer { text-align: center; margin-top: 20px; color: #7f8c8d; }";
    css += "</style>";

    client.print(css);  // Single print instead of 40+ println() calls
}


void WebServer::logEvent(const String& event, const String& status) {
    _logs[_logIndex] = {millis(), event, status};
    _logIndex = (_logIndex + 1) % MAX_LOGS;
}

void WebServer::logResponseTime(unsigned long startTime) {
    unsigned long responseTime = millis() - startTime;

    Serial.print("Response time: ");
    Serial.print(responseTime);
    Serial.println("ms");

    if (responseTime > 1000) {  // Warn if > 1 second
        Serial.println("WARNING: Slow response detected!");
        logEvent("Performance", "Slow response: " + String(responseTime) + "ms");
    }
}

// Optimized recent logs method
String WebServer::getRecentLogsOptimized() {
    String logs;
    logs.reserve(512);

    logs += "<h2>Recent Activity</h2>";
    logs += "<div class=\"log-container\">";

    // Show recent logs efficiently
    for (int i = 0; i < MAX_LOGS; i++) {
        int idx = (_logIndex - 1 - i + MAX_LOGS) % MAX_LOGS;
        if (_logs[idx].timestamp > 0) {
            logs += "<div class=\"log-entry\">";
            logs += _logs[idx].event + " - " + _logs[idx].status;
            logs += "</div>";
        }
    }

    logs += "</div>";
    return logs;
}

// Add this optimized AJAX JavaScript method to WebServer.cpp
String WebServer::getOptimizedAjaxScript() {
    String js;
    js.reserve(1024);  // Pre-allocate for performance

    js += "<script>";

    // Main command function - optimized and compressed
    js += "function cmd(c){";
    js += "const b=event.target;";
    js += "const orig=b.innerHTML;";
    js += "b.disabled=1;";
    js += "b.innerHTML='⏳';";
    js += "b.style.opacity='0.7';";
    js += "fetch('/api/control?cmd='+c)";
    js += ".then(r=>r.json())";
    js += ".then(d=>{";
    js += "msg(d.success?'✅ '+d.message:'❌ '+d.message,d.success);";
    js += "b.disabled=0;b.innerHTML=orig;b.style.opacity='1';";
    js += "if(d.success)setTimeout(()=>updateStatus(),500);";  // Refresh status after success
    js += "})";
    js += ".catch(e=>{";
    js += "msg('❌ Connection failed',0);";
    js += "b.disabled=0;b.innerHTML=orig;b.style.opacity='1';";
    js += "});";
    js += "}";

    // Message display function
    js += "function msg(t,success){";
    js += "const d=document.createElement('div');";
    js += "d.innerHTML=t;";
    js += "d.style.cssText='position:fixed;top:20px;right:20px;padding:12px 16px;color:#fff;border-radius:6px;z-index:1000;font-weight:bold;box-shadow:0 4px 6px rgba(0,0,0,0.1);';";
    js += "d.style.backgroundColor=success?'#27ae60':'#e74c3c';";
    js += "document.body.appendChild(d);";
    js += "setTimeout(()=>{d.style.opacity='0';d.style.transform='translateX(100%)';d.style.transition='all 0.3s';setTimeout(()=>d.remove(),300);},2500);";
    js += "}";

    // Status update function for live data
    js += "function updateStatus(){";
    js += "fetch('/api/status')";
    js += ".then(r=>r.json())";
    js += ".then(d=>{";
    js += "const items=document.querySelectorAll('.status-item');";
    js += "if(items[0])items[0].innerHTML='<strong>State:</strong><br>'+d.state;";
    js += "if(items[1])items[1].innerHTML='<strong>Speed:</strong><br>'+d.speed;";
    js += "if(items[2])items[2].innerHTML='<strong>Safety:</strong><br>'+d.safety;";
    js += "if(items[3])items[3].innerHTML='<strong>Uptime:</strong><br>'+d.uptime+'s';";
    js += "})";
    js += ".catch(e=>console.log('Status update failed'));";
    js += "}";

    // Auto-refresh status every 10 seconds (less aggressive than before)
    js += "setInterval(updateStatus,10000);";

    js += "</script>";

    return js;
}


String WebServer::getSensorData() {
    String data;
    data.reserve(1024);  // Pre-allocate for performance

    data += "<div class=\"status-grid\">";

    // User presence - using existing SafetyMonitor method
    data += "<div class=\"status-item\"><strong>User Present:</strong><br>";
    data += (_safetyMonitor->isUserPresent() ? "<span style=\"color: green\">YES</span>" : "<span style=\"color: red\">NO</span>");
    data += "</div>";

    // frontLeft distance
    data += "<div class=\"status-item\"><strong>frontLeft Distance:</strong><br>";
    float frontLeftDist = _safetyMonitor->getFrontLeftDistance();
    if (frontLeftDist > 0 && frontLeftDist < 500) {
        data += String(frontLeftDist, 1) + " cm";
    } else {
        data += "No reading";
    }
    data += "</div>";

    // frontRight distance
    data += "<div class=\"status-item\"><strong>frontRight Distance:</strong><br>";
    float frontRightDist = _safetyMonitor->getFrontRightDistance();
    if (frontRightDist > 0 && frontRightDist < 500) {
        data += String(frontRightDist, 1) + " cm";
    } else {
        data += "No reading";
    }
    data += "</div>";

    // Safety status with proper color coding
    data += "<div class=\"status-item\"><strong>Safety Status:</strong><br>";
    String statusStr = _safetyMonitor->getStatusString();
    if (statusStr == "SAFE") {
        data += "<span style=\"color: green\">✅ " + statusStr + "</span>";
    } else if (statusStr == "WARNING") {
        data += "<span style=\"color: orange\">⚠️ " + statusStr + "</span>";
    } else if (statusStr == "ERROR") {
        data += "<span style=\"color: red\">❌ " + statusStr + "</span>";
    } else if (statusStr == "EMERGENCY") {
        data += "<span style=\"color: red; font-weight: bold\">🚨 " + statusStr + "</span>";
    } else {
        data += statusStr;
    }
    data += "</div>";

    // Current physics settings (new addition)
    RuntimeConfig& config = RuntimeConfig::getInstance();
    data += "<div class=\"status-item\"><strong>Physics:</strong><br>";
    data += String(config.getPushDurationPercent()) + "% push, " + String(config.getPushPowerPercent()) + "% power";
    data += "</div>";

    data += "</div>";
    return data;
}



String WebServer::getSystemStatus() {
    String status = "State: ";
    status += _stateMachine->getStateString();
    status += ", Speed: ";
    status += _stateMachine->getSpeedString();
    return status;
}
