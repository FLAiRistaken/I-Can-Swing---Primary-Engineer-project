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

    client.stop();
}

// ===== PAGE HANDLERS =====

void WebServer::sendHomePage(WiFiClient& client) {
    sendPageHeader(client, "I Can Swing - Home");

    client.println("<h2>System Status</h2>");
    client.println("<div class='status-grid'>");

    // System state
    client.print("<div class='status-item'><strong>State:</strong> ");
    client.print(_stateMachine->getStateString());
    client.println("</div>");

    client.print("<div class='status-item'><strong>Speed:</strong> ");
    client.print(_stateMachine->getSpeedString());
    client.println("</div>");

    // Sensor data
    String sensorData = getSensorData();
    client.println(sensorData);

    client.println("</div>");

    // Quick actions
    client.println("<h2>Quick Actions</h2>");
    client.println("<div class='button-grid'>");
    client.println("<a href='/control' class='btn btn-primary'>Control Panel</a>");
    client.println("<a href='/config' class='btn btn-secondary'>Configuration</a>");
    client.println("<a href='#' onclick='refresh()' class='btn btn-info'>Refresh</a>");
    client.println("</div>");

    // Recent logs
    client.println("<h2>Recent Activity</h2>");
    client.println("<div class='log-container'>");
    for (int i = 0; i < MAX_LOGS; i++) {
        int idx = (_logIndex - 1 - i + MAX_LOGS) % MAX_LOGS;
        if (_logs[idx].timestamp > 0) {
            client.print("<div class='log-entry'>");
            client.print(_logs[idx].event);
            client.print(": ");
            client.print(_logs[idx].status);
            client.println("</div>");
        }
    }
    client.println("</div>");

    // Auto-refresh script
    client.println("<script>");
    client.println("function refresh() { location.reload(); }");
    client.println("setInterval(refresh, 10000);"); // Auto-refresh every 10 seconds
    client.println("</script>");

    sendPageFooter(client);
}

void WebServer::sendControlPage(WiFiClient& client) {
    sendPageHeader(client, "I Can Swing - Control");

    client.println("<h2>System Control</h2>");
    client.println("<p>Use these controls to operate the swing system.</p>");

    // Main controls
    client.println("<div class='control-section'>");
    client.println("<h3>Main Controls</h3>");
    client.println("<div class='button-grid'>");

    if (_stateMachine->getCurrentState() == StateMachine::STATE_IDLE) {
        client.println("<button onclick='sendCommand(\"start\")' class='btn btn-success btn-large'>START SWING</button>");
    } else if (_stateMachine->getCurrentState() == StateMachine::STATE_SWINGING) {
        client.println("<button onclick='sendCommand(\"stop\")' class='btn btn-warning btn-large'>STOP SWING</button>");
    } else {
        client.println("<button disabled class='btn btn-secondary btn-large'>SYSTEM BUSY</button>");
    }

    client.println("<button onclick='sendCommand(\"emergency\")' class='btn btn-danger btn-large'>EMERGENCY STOP</button>");
    client.println("</div>");
    client.println("</div>");

    // Speed controls
    client.println("<div class='control-section'>");
    client.println("<h3>Speed Control</h3>");
    client.println("<div class='button-grid'>");
    client.println("<button onclick='sendCommand(\"speed_low\")' class='btn btn-info'>Low Speed</button>");
    client.println("<button onclick='sendCommand(\"speed_medium\")' class='btn btn-info'>Medium Speed</button>");
    client.println("<button onclick='sendCommand(\"speed_high\")' class='btn btn-info'>High Speed</button>");
    client.println("</div>");
    client.println("</div>");

    // Door controls
    client.println("<div class='control-section'>");
    client.println("<h3>Door Control</h3>");
    client.println("<div class='button-grid'>");
    client.println("<button onclick='sendCommand(\"door_open\")' class='btn btn-primary'>Open Door</button>");
    client.println("<button onclick='sendCommand(\"door_close\")' class='btn btn-primary'>Close Door</button>");
    client.println("</div>");
    client.println("</div>");

    // Audio controls
    client.println("<div class='control-section'>");
    client.println("<h3>Audio Feedback</h3>");
    client.println("<div class='button-grid'>");
    client.println("<button onclick='sendCommand(\"alert\")' class='btn btn-secondary'>Alert Tone</button>");
    client.println("<button onclick='sendCommand(\"melody\")' class='btn btn-secondary'>Give Melody</button>");
    client.println("</div>");
    client.println("</div>");

    // JavaScript for control commands
    client.println("<script>");
    client.println("function sendCommand(cmd) {");
    client.println("  fetch('/api/control?cmd=' + cmd)");
    client.println("    .then(response => response.json())");
    client.println("    .then(data => {");
    client.println("      alert(data.message);");
    client.println("      setTimeout(() => location.reload(), 1000);");
    client.println("    })");
    client.println("    .catch(err => alert('Error: ' + err));");
    client.println("}");
    client.println("</script>");

    sendPageFooter(client);
}

void WebServer::sendConfigPage(WiFiClient& client) {
    sendPageHeader(client, "I Can Swing - Configuration");

    RuntimeConfig& config = RuntimeConfig::getInstance();

    client.println("<h2>System Configuration</h2>");
    client.println("<form onsubmit='saveConfig(event)'>");

    // Safety settings
    client.println("<div class='config-section'>");
    client.println("<h3>Safety Settings</h3>");

    client.print("<label>Warning Distance (cm): <input type='number' name='warningDistance' value='");
    client.print(config.getFrontWarningDistance());
    client.println("' min='10' max='200'></label>");

    client.print("<label>Critical Distance (cm): <input type='number' name='criticalDistance' value='");
    client.print(config.getFrontCriticalDistance());
    client.println("' min='5' max='50'></label>");

    client.print("<label>Pressure Threshold: <input type='number' name='pressureThreshold' value='");
    client.print(config.getPressureThreshold());
    client.println("' min='100' max='900'></label>");

    client.println("</div>");

    // Swing settings
    client.println("<div class='config-section'>");
    client.println("<h3>Swing Motion Settings</h3>");

    client.print("<label>Swing Period (ms): <input type='number' name='swingPeriod' value='");
    client.print(config.getSwingPeriodMs());
    client.println("' min='3000' max='8000'></label>");

    client.print("<label>Max Swing Angle (degrees): <input type='number' name='swingAngle' value='");
    client.print(config.getSwingMaxAngleDegrees());
    client.println("' min='20' max='60'></label>");

    client.print("<label>Smooth Stop Time (ms): <input type='number' name='smoothStop' value='");
    client.print(config.getSwingSmoothStopMs());
    client.println("' min='1000' max='5000'></label>");

    client.println("</div>");

    // System settings
    client.println("<div class='config-section'>");
    client.println("<h3>System Settings</h3>");

    client.print("<label>Door Timeout (ms): <input type='number' name='doorTimeout' value='");
    client.print(config.getDoorTimeoutMs());
    client.println("' min='5000' max='30000'></label>");

    client.print("<label>Buzzer Volume (0-10): <input type='number' name='buzzerVolume' value='");
    client.print(config.getBuzzerVolume());
    client.println("' min='0' max='10'></label>");

    client.print("<label><input type='checkbox' name='audioFeedback'");
    if (config.isAudioFeedbackEnabled()) client.print(" checked");
    client.println("> Audio Feedback Enabled</label>");

    client.print("<label><input type='checkbox' name='voiceRecognition'");
    if (config.isVoiceRecognitionEnabled()) client.print(" checked");
    client.println("> Voice Recognition Enabled</label>");

    client.println("</div>");

    client.println("<div class='button-grid'>");
    client.println("<button type='submit' class='btn btn-success'>Save Configuration</button>");
    client.println("<button type='button' onclick='loadDefaults()' class='btn btn-warning'>Load Defaults</button>");
    client.println("</div>");

    client.println("</form>");

    // Configuration JavaScript
    client.println("<script>");
    client.println("function saveConfig(event) {");
    client.println("  event.preventDefault();");
    client.println("  const form = event.target;");
    client.println("  const formData = new FormData(form);");
    client.println("  const params = new URLSearchParams(formData).toString();");
    client.println("  fetch('/api/config?' + params)");
    client.println("    .then(response => response.json())");
    client.println("    .then(data => {");
    client.println("      alert(data.message);");
    client.println("      if (data.success) setTimeout(() => location.reload(), 1000);");
    client.println("    })");
    client.println("    .catch(err => alert('Error: ' + err));");
    client.println("}");
    client.println("function loadDefaults() {");
    client.println("  if (confirm('Load default settings? This will overwrite current configuration.')) {");
    client.println("    fetch('/api/config?action=defaults')");
    client.println("      .then(response => response.json())");
    client.println("      .then(data => {");
    client.println("        alert(data.message);");
    client.println("        location.reload();");
    client.println("      });");
    client.println("  }");
    client.println("}");
    client.println("</script>");

    sendPageFooter(client);
}

void WebServer::send404Page(WiFiClient& client) {
    sendHttpHeader(client);
    client.println("<html><head><title>404 Not Found</title></head>");
    client.println("<body><h1>404 - Page Not Found</h1>");
    client.println("<a href='/'>Return to Home</a></body></html>");
}

// ===== API HANDLERS =====

void WebServer::handleControlAPI(WiFiClient& client, String command) {
    String response = "{";
    bool success = true;
    String message = "";

    if (command == "start") {
        _stateMachine->processEvent(StateMachine::EVENT_START_PRESSED);
        message = "Start command sent";
    } else if (command == "stop") {
        _stateMachine->processEvent(StateMachine::EVENT_STOP_PRESSED);
        message = "Stop command sent";
    } else if (command == "emergency") {
        _stateMachine->processEvent(StateMachine::EVENT_EMERGENCY);
        message = "Emergency stop activated";
    } else if (command == "speed_low") {
        _stateMachine->processEvent(StateMachine::EVENT_SPEED_SET_LOW);
        message = "Speed set to LOW";
    } else if (command == "speed_medium") {
        _stateMachine->processEvent(StateMachine::EVENT_SPEED_SET_MEDIUM);
        message = "Speed set to MEDIUM";
    } else if (command == "speed_high") {
        _stateMachine->processEvent(StateMachine::EVENT_SPEED_SET_HIGH);
        message = "Speed set to HIGH";
    } else if (command == "door_open") {
        _stateMachine->processEvent(StateMachine::EVENT_DOOR_OPEN_PRESSED);
        message = "Door open command sent";
    } else if (command == "door_close") {
        _stateMachine->processEvent(StateMachine::EVENT_DOOR_CLOSE_PRESSED);
        message = "Door close command sent";
    } else if (command == "alert") {
        _stateMachine->processEvent(StateMachine::EVENT_ALERT_PRESSED);
        message = "Alert tone played";
    } else if (command == "melody") {
        _stateMachine->processEvent(StateMachine::EVENT_GIVE_MELODY_PRESSED);
        message = "Give melody played";
    } else {
        success = false;
        message = "Unknown command: " + command;
    }

    response += "\"success\": " + String(success ? "true" : "false") + ",";
    response += "\"message\": \"" + message + "\"";
    response += "}";

    logEvent("Control", command + " -> " + message);
    sendJsonResponse(client, response);
}

void WebServer::handleConfigAPI(WiFiClient& client, String params) {
    RuntimeConfig& config = RuntimeConfig::getInstance();
    String response = "{";
    bool success = true;
    String message = "";

    if (params.indexOf("action=defaults") >= 0) {
        config.loadDefaultPreset();
        message = "Default configuration loaded";
    } else {
        // Parse and apply configuration parameters
        if (params.indexOf("warningDistance=") >= 0) {
            int start = params.indexOf("warningDistance=") + 16;
            int end = params.indexOf("&", start);
            if (end == -1) end = params.length();
            float value = params.substring(start, end).toFloat();
            if (!config.setWarningDistance(value)) {
                success = false;
                message = "Invalid warning distance";
            }
        }

        if (params.indexOf("criticalDistance=") >= 0) {
            int start = params.indexOf("criticalDistance=") + 17;
            int end = params.indexOf("&", start);
            if (end == -1) end = params.length();
            float value = params.substring(start, end).toFloat();
            if (!config.setCriticalDistance(value)) {
                success = false;
                message = "Invalid critical distance";
            }
        }

        if (params.indexOf("pressureThreshold=") >= 0) {
            int start = params.indexOf("pressureThreshold=") + 18;
            int end = params.indexOf("&", start);
            if (end == -1) end = params.length();
            uint16_t value = params.substring(start, end).toInt();
            if (!config.setPressureThreshold(value)) {
                success = false;
                message = "Invalid pressure threshold";
            }
        }

        if (params.indexOf("doorTimeout=") >= 0) {
            int start = params.indexOf("doorTimeout=") + 12;
            int end = params.indexOf("&", start);
            if (end == -1) end = params.length();
            unsigned long value = params.substring(start, end).toInt();
            if (!config.setDoorTimeoutMs(value)) {
                success = false;
                message = "Invalid door timeout";
            }
        }

        if (params.indexOf("buzzerVolume=") >= 0) {
            int start = params.indexOf("buzzerVolume=") + 13;
            int end = params.indexOf("&", start);
            if (end == -1) end = params.length();
            uint8_t value = params.substring(start, end).toInt();
            if (!config.setBuzzerVolume(value)) {
                success = false;
                message = "Invalid buzzer volume";
            }
        }

        // Handle checkboxes
        config.setAudioFeedbackEnabled(params.indexOf("audioFeedback=on") >= 0);
        config.setVoiceRecognitionEnabled(params.indexOf("voiceRecognition=on") >= 0);

        if (success) {
            config.save();
            message = "Configuration saved successfully";
        }
    }

    response += "\"success\": " + String(success ? "true" : "false") + ",";
    response += "\"message\": \"" + message + "\"";
    response += "}";

    logEvent("Config", message);
    sendJsonResponse(client, response);
}

void WebServer::handleStatusAPI(WiFiClient& client) {
    String response = "{";
    response += "\"state\": \"" + String(_stateMachine->getStateString()) + "\",";
    response += "\"speed\": \"" + String(_stateMachine->getSpeedString()) + "\",";
    response += "\"sensorData\": " + getSensorData();
    response += "}";

    sendJsonResponse(client, response);
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
    sendHttpHeader(client);
    client.println("<!DOCTYPE html>");
    client.println("<html><head>");
    client.print("<title>");
    client.print(title);
    client.println("</title>");
    client.println("<meta name='viewport' content='width=device-width, initial-scale=1'>");
    sendSimpleCSS(client);
    client.println("</head><body>");
    client.println("<header>");
    client.println("<h1>I Can Swing Control System</h1>");
    client.println("<nav>");
    client.println("<a href='/'>Home</a> | ");
    client.println("<a href='/control'>Control</a> | ");
    client.println("<a href='/config'>Configuration</a>");
    client.println("</nav>");
    client.println("</header>");
    client.println("<main>");
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
    client.println("<style>");
    client.println("body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background: #f5f5f5; }");
    client.println("header { background: #2c3e50; color: white; padding: 15px; border-radius: 5px; margin-bottom: 20px; }");
    client.println("header h1 { margin: 0; }");
    client.println("nav { margin-top: 10px; }");
    client.println("nav a { color: #ecf0f1; text-decoration: none; margin: 0 10px; }");
    client.println("main { background: white; padding: 20px; border-radius: 5px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }");
    client.println(".button-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); gap: 10px; margin: 15px 0; }");
    client.println(".btn { padding: 12px 20px; border: none; border-radius: 5px; text-decoration: none; text-align: center; cursor: pointer; font-size: 14px; }");
    client.println(".btn-large { padding: 20px; font-size: 18px; font-weight: bold; }");
    client.println(".btn-primary { background: #3498db; color: white; }");
    client.println(".btn-success { background: #27ae60; color: white; }");
    client.println(".btn-warning { background: #f39c12; color: white; }");
    client.println(".btn-danger { background: #e74c3c; color: white; }");
    client.println(".btn-info { background: #17a2b8; color: white; }");
    client.println(".btn-secondary { background: #6c757d; color: white; }");
    client.println(".status-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; margin: 15px 0; }");
    client.println(".status-item { padding: 10px; background: #ecf0f1; border-radius: 5px; }");
    client.println(".control-section { margin: 20px 0; padding: 15px; border: 1px solid #ddd; border-radius: 5px; }");
    client.println(".config-section { margin: 20px 0; padding: 15px; border: 1px solid #ddd; border-radius: 5px; }");
    client.println("label { display: block; margin: 10px 0; }");
    client.println("input[type='number'] { width: 100px; padding: 5px; }");
    client.println("input[type='checkbox'] { margin-right: 5px; }");
    client.println(".log-container { max-height: 200px; overflow-y: auto; border: 1px solid #ddd; padding: 10px; }");
    client.println(".log-entry { padding: 5px; border-bottom: 1px solid #eee; }");
    client.println("footer { text-align: center; margin-top: 20px; color: #7f8c8d; }");
    client.println("</style>");
}

void WebServer::logEvent(const String& event, const String& status) {
    _logs[_logIndex] = {millis(), event, status};
    _logIndex = (_logIndex + 1) % MAX_LOGS;
}

String WebServer::getSensorData() {
    String data = "";

    if (_safetyMonitor) {
        // User presence from SafetyMonitor
        data += "<div class='status-item'><strong>User Present:</strong> ";
        data += _safetyMonitor->isUserPresent() ? "<span style='color: green;'>YES</span>" : "<span style='color: red;'>NO</span>";
        data += "</div>";

        // Front ultrasonic sensor
        data += "<div class='status-item'><strong>Front Distance:</strong> ";
        float frontDist = _safetyMonitor->getFrontDistance();
        if (frontDist > 0 && frontDist < 500) {
            data += String(frontDist, 1) + " cm";
        } else {
            data += "No reading";
        }
        data += "</div>";

        // Rear ultrasonic sensor
        data += "<div class='status-item'><strong>Rear Distance:</strong> ";
        float rearDist = _safetyMonitor->getRearDistance();
        if (rearDist > 0 && rearDist < 500) {
            data += String(rearDist, 1) + " cm";
        } else {
            data += "No reading";
        }
        data += "</div>";

        // Safety status with color coding
        data += "<div class='status-item'><strong>Safety Status:</strong> ";
        String status = _safetyMonitor->getStatusString();
        if (status == "SAFE") {
            data += "<span style='color: green;'>SAFE</span>";
        } else if (status == "WARNING") {
            data += "<span style='color: orange;'>WARNING</span>";
        } else if (status == "ERROR") {
            data += "<span style='color: red;'>ERROR</span>";
        } else if (status == "EMERGENCY") {
            data += "<span style='color: red; font-weight: bold;'>EMERGENCY</span>";
        } else {
            data += status;
        }
        data += "</div>";

        // Dynamic safety thresholds (shows current effective thresholds)
        data += "<div class='status-item'><strong>Warning Threshold:</strong> ";
        data += String(_safetyMonitor->getEffectiveWarningDistance(), 1) + " cm</div>";

        data += "<div class='status-item'><strong>Critical Threshold:</strong> ";
        data += String(_safetyMonitor->getEffectiveCriticalDistance(), 1) + " cm</div>";

    } else {
        data += "<div class='status-item'><strong>Sensors:</strong> <span style='color: red;'>Not Available</span></div>";
    }

    // System information
    data += "<div class='status-item'><strong>Uptime:</strong> ";
    unsigned long seconds = millis() / 1000;
    unsigned long hours = seconds / 3600;
    unsigned long minutes = (seconds % 3600) / 60;
    seconds = seconds % 60;
    data += String(hours) + "h " + String(minutes) + "m " + String(seconds) + "s</div>";

    // Memory usage (ESP32 specific)
    data += "<div class='status-item'><strong>Free Memory:</strong> ";
    data += "Available</div>"; // Simple placeholder

    return data;
}


String WebServer::getSystemStatus() {
    String status = "State: ";
    status += _stateMachine->getStateString();
    status += ", Speed: ";
    status += _stateMachine->getSpeedString();
    return status;
}
