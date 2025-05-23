// lib/Connectivity/WebServer.cpp
#include "WebServer.h"

WebServer::WebServer(StateMachine* stateMachine, SafetyMonitor* safetyMonitor)
    : _server(80), _stateMachine(stateMachine), _safetyMonitor(safetyMonitor) {}

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
                    // End of HTTP request
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
        } else if (request.indexOf("GET /api/") >= 0) {
            // Handle API commands
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

void WebServer::sendHomePage(WiFiClient& client) {
    sendHttpHeader(client);

    client.println("<!DOCTYPE html>");
    client.println("<html><head>");
    client.println("<title>Wheelchair Swing Admin Panel</title>");
    client.println("<meta name='viewport' content='width=device-width, initial-scale=1'>");
    client.println("<style>");
    client.println("body { font-family: Arial; margin: 20px; background-color: #f0f0f0; }");
    client.println(".container { max-width: 800px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; }");
    client.println(".status-box { background: #e8f5e8; padding: 15px; margin: 10px 0; border-radius: 5px; }");
    client.println(".nav-button { display: inline-block; background: #007bff; color: white; padding: 10px 20px; text-decoration: none; margin: 5px; border-radius: 5px; }");
    client.println(".emergency { background: #ff4444 !important; }");
    client.println("</style></head><body>");

    client.println("<div class='container'>");
    client.println("<h1>🎠 Wheelchair Swing Control Panel</h1>");

    // System Status
    client.println("<div class='status-box'>");
    client.println("<h3>System Status</h3>");
    client.print("<p><strong>State:</strong> ");
    client.println(_stateMachine->getStateString());
    client.print("<p><strong>Speed:</strong> ");
    client.println(_stateMachine->getSpeedString());
    client.print("<p><strong>Safety:</strong> ");
    client.println(_safetyMonitor->getStatusString());
    client.print("<p><strong>Front Distance:</strong> ");
    client.print(_safetyMonitor->getFrontDistance());
    client.println(" cm</p>");
    client.print("<p><strong>Rear Distance:</strong> ");
    client.print(_safetyMonitor->getRearDistance());
    client.println(" cm</p>");
    client.print("<p><strong>User Present:</strong> ");
    client.println(_safetyMonitor->isUserPresent() ? "Yes" : "No");
    client.println("</div>");

    // Navigation
    client.println("<h3>Navigation</h3>");
    client.println("<a href='/control' class='nav-button'>🎮 Control Panel</a>");
    client.println("<a href='/status' class='nav-button'>📊 Status Monitor</a>");
    client.println("<a href='/config' class='nav-button'>⚙️ Configuration</a>");
    client.println("<a href='/debug' class='nav-button'>🔧 Debug Tools</a>");

    // Quick Controls
    client.println("<h3>Quick Actions</h3>");
    if (_stateMachine->getCurrentState() == StateMachine::STATE_EMERGENCY) {
        client.println("<a href='/api/reset' class='nav-button emergency'>🚨 EMERGENCY RESET</a>");
    } else {
        client.println("<a href='/api/start' class='nav-button'>▶️ Start</a>");
        client.println("<a href='/api/stop' class='nav-button'>⏹️ Stop</a>");
        client.println("<a href='/api/emergency' class='nav-button emergency'>🚨 Emergency Stop</a>");
    }

    client.println("</div>");
    client.println("<script>setTimeout(() => location.reload(), 5000);</script>"); // Auto-refresh
    client.println("</body></html>");
}

void WebServer::sendControlPage(WiFiClient& client) {
    sendHttpHeader(client);

    client.println("<!DOCTYPE html>");
    client.println("<html><head>");
    client.println("<title>Control Panel - Wheelchair Swing</title>");
    client.println("<meta name='viewport' content='width=device-width, initial-scale=1'>");
    client.println("<style>");
    client.println("body { font-family: Arial; margin: 20px; background-color: #f0f0f0; }");
    client.println(".container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; }");
    client.println(".control-button { display: block; width: 100%; background: #007bff; color: white; padding: 15px; text-decoration: none; margin: 10px 0; border-radius: 5px; text-align: center; font-size: 18px; }");
    client.println(".emergency { background: #ff4444 !important; }");
    client.println(".success { background: #28a745 !important; }");
    client.println("</style></head><body>");

    client.println("<div class='container'>");
    client.println("<h1>🎮 Control Panel</h1>");
    client.println("<p><a href='/'>← Back to Home</a></p>");

    client.println("<h3>Basic Controls</h3>");
    client.println("<a href='/api/start' class='control-button success'>▶️ START SWING</a>");
    client.println("<a href='/api/stop' class='control-button'>⏹️ STOP SWING</a>");

    client.println("<h3>Speed Controls</h3>");
    client.println("<a href='/api/speed-up' class='control-button'>⬆️ INCREASE SPEED</a>");
    client.println("<a href='/api/speed-down' class='control-button'>⬇️ DECREASE SPEED</a>");

    client.println("<h3>Door Controls</h3>");
    client.println("<a href='/api/door-open' class='control-button'>🚪 OPEN DOOR</a>");
    client.println("<a href='/api/door-close' class='control-button'>🚪 CLOSE DOOR</a>");

    client.println("<h3>Safety Controls</h3>");
    client.println("<a href='/api/emergency' class='control-button emergency'>🚨 EMERGENCY STOP</a>");
    if (_stateMachine->getCurrentState() == StateMachine::STATE_EMERGENCY) {
        client.println("<a href='/api/reset' class='control-button'>🔄 RESET FROM EMERGENCY</a>");
    }
    if (_stateMachine->getCurrentState() == StateMachine::STATE_ERROR) {
        client.println("<a href='/api/clear-error' class='control-button'>✅ CLEAR ERROR</a>");
    }

    client.println("</div>");
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
