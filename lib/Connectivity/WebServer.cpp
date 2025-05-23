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
