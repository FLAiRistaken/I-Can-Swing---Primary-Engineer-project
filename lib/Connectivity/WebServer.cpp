// lib/Connectivity/WebServer.cpp
#include "WebServer.h"

WebServer::WebServer(StateMachine* stateMachine, SafetyMonitor* safetyMonitor)
    : _server(80), _stateMachine(stateMachine), _safetyMonitor(safetyMonitor),
      _calibrationActive(false), _calibrationStartTime(0), _calibrationStep(0),
      _currentSensorCalibrating(""), _motorTestActive(false), _currentMotorTest(""),
      _motorTestStartTime(0), _motorTestStep(0), _leftMotorPosition(0),
      _rightMotorPosition(0), _motorTestSafetyCheck(true) {}

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

String WebServer::getSystemStatus() {
    return String(_stateMachine->getStateString());
}

String WebServer::getCurrentDateTime() {
    return String(millis() / 1000);
}
