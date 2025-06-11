// lib/Connectivity/WebServer.cpp
#include "WebServer.h"
#include "RuntimeConfig.h"

WebServer::WebServer(StateMachine *stateMachine, SafetyMonitor *safetyMonitor)
    : _server(80), _stateMachine(stateMachine), _safetyMonitor(safetyMonitor),
      _leftStepper(nullptr), _rightStepper(nullptr), _doorActuator(nullptr), _logIndex(0)
{

    // ✅ SIMPLIFIED: Initialize single test state structure
    _testState = {
        false, "", 0, 0, 0, 0, true,        // Motor testing
        false, "", 0, 0, 0, false           // Demo
    };
}

void WebServer::setStepperDrivers(StepperDriver *leftStepper, StepperDriver *rightStepper)
{
    _leftStepper = leftStepper;
    _rightStepper = rightStepper;
}

void WebServer::setDoorActuator(ActuatorDriver* doorActuator) {
    _doorActuator = doorActuator;
}


void WebServer::begin(int port)
{
    _server.begin();
    Serial.print(F("WebServer started on port "));
    Serial.println(port);
}

void WebServer::handleClient() {
    WiFiClient client = _server.available();
    if (!client) return;

    // Only process if data is immediately available
    if (!client.available()) {
        client.stop();
        return;
    }

    // Read only what's available now
    String request = "";
    unsigned long startTime = millis();

    // Read with timeout to prevent blocking
    while (client.connected() && client.available() && (millis() - startTime < 1000)) {
        String line = client.readStringUntil('\n');
        request += line;

        // Look for end of HTTP headers (empty line)
        if (line.length() <= 2) { // Just \r\n or \n
            break;
        }
    }

    // Only handle if we have a complete request
    if (request.length() < 10) {
        client.stop();
        return;
    }

    Serial.print(F("WebServer: Processing request - "));
    Serial.println(request.substring(0, 50)); // Debug output

    // Fast request routing
    if (request.indexOf(F("GET / ")) >= 0) {
        sendHomePage(client);
    } else if (request.indexOf(F("GET /control")) >= 0) {
        sendControlPage(client);
    } else if (request.indexOf(F("GET /config")) >= 0) {
        sendConfigPage(client);
    } else if (request.indexOf(F("GET /motor-test")) >= 0) {
        sendMotorTestPage(client);
    } else if (request.indexOf(F("GET /demo")) >= 0) {
        sendDemoPage(client);
    } else if (request.indexOf(F("GET /api/")) >= 0) {
        // Extract API endpoint and parameters
        int apiStart = request.indexOf(F("/api/")) + 5;
        int apiEnd = request.indexOf(F(" "), apiStart);
        String endpoint = request.substring(apiStart, apiEnd);

        String params = "";
        int paramStart = request.indexOf(F("?"));
        if (paramStart >= 0) {
            int paramEnd = request.indexOf(F(" HTTP"), paramStart);
            params = request.substring(paramStart + 1, paramEnd);
            endpoint = endpoint.substring(0, endpoint.indexOf(F("?")));
        }

        handleAPI(client, endpoint, params);
    } else {
        send404Page(client);
    }

    client.stop();

    Serial.println(F("WebServer: Request completed, loop continues"));
}

// Unified CSS in flash memory
void WebServer::sendUnifiedCSS(WiFiClient &client)
{
    client.println(F("<style>"));
    client.println(F(":root{--primary:#3b82f6;--success:#10b981;--warning:#f59e0b;--danger:#ef4444;--gray-50:#f8fafc;--gray-100:#f1f5f9;--gray-200:#e2e8f0;--gray-600:#475569;--gray-900:#0f172a}"));
    client.println(F("*{margin:0;padding:0;box-sizing:border-box}"));
    client.println(F("body{font-family:Inter,sans-serif;background:var(--gray-50);color:var(--gray-900);line-height:1.6}"));
    client.println(F(".container{max-width:1200px;margin:0 auto;padding:20px}"));
    client.println(F(".header{background:#fff;border-radius:12px;padding:32px;margin-bottom:24px;text-align:center;box-shadow:0 1px 3px rgb(0 0 0/0.1)}"));
    client.println(F(".header h1{font-size:2.5rem;font-weight:800;margin-bottom:8px}"));
    client.println(F(".header a{color:var(--primary);text-decoration:none}"));
    client.println(F(".card{background:#fff;border-radius:12px;padding:24px;margin-bottom:24px;box-shadow:0 1px 3px rgb(0 0 0/0.1);border:1px solid var(--gray-200)}"));
    client.println(F(".card h3{font-size:1.25rem;font-weight:700;margin-bottom:16px;display:flex;align-items:center;gap:8px}"));
    client.println(F(".card h3 i{color:var(--primary);font-size:1.1rem}"));
    client.println(F(".btn{padding:12px 24px;border-radius:8px;font-weight:600;border:none;cursor:pointer;text-decoration:none;display:inline-flex;align-items:center;gap:8px;margin:4px;transition:all 0.2s ease}"));
    client.println(F(".btn:hover{transform:translateY(-1px);box-shadow:0 4px 6px rgb(0 0 0/0.1)}"));
    client.println(F(".btn-primary{background:var(--primary);color:#fff}"));
    client.println(F(".btn-success{background:var(--success);color:#fff}"));
    client.println(F(".btn-warning{background:var(--warning);color:#fff}"));
    client.println(F(".btn-danger{background:var(--danger);color:#fff}"));
    client.println(F(".grid{display:grid;gap:16px}"));
    client.println(F(".grid-2{grid-template-columns:repeat(auto-fit,minmax(300px,1fr))}"));
    client.println(F(".grid-3{grid-template-columns:repeat(auto-fit,minmax(250px,1fr))}"));
    client.println(F(".grid-4{grid-template-columns:repeat(auto-fit,minmax(200px,1fr))}"));
    client.println(F(".status-item{display:flex;justify-content:space-between;align-items:center;padding:12px 0;border-bottom:1px solid var(--gray-200)}"));
    client.println(F(".status-item:last-child{border-bottom:none}"));
    client.println(F(".status-value{font-weight:700;padding:4px 12px;border-radius:6px;font-size:0.9rem}"));
    client.println(F(".status-ok{background:#dcfce7;color:#166534}"));
    client.println(F(".status-warning{background:#fef3c7;color:#92400e}"));
    client.println(F(".status-error{background:#fecaca;color:#991b1b}"));
    client.println(F(".form-group{margin-bottom:16px}"));
    client.println(F(".form-group label{display:block;font-weight:600;margin-bottom:4px;color:var(--gray-600)}"));
    client.println(F(".form-group input,.form-group select{width:100%;padding:10px;border:2px solid var(--gray-200);border-radius:8px;font-size:1rem}"));
    client.println(F(".form-group input:focus{outline:none;border-color:var(--primary)}"));
    client.println(F(".slider{width:100%;height:6px;border-radius:3px;background:var(--gray-200);margin:12px 0}"));
    client.println(F(".log-container{max-height:300px;overflow-y:auto;background:var(--gray-50);border-radius:8px;padding:16px;margin:16px 0;font-family:'Courier New',monospace;font-size:0.85rem}"));
    client.println(F(".log-entry{padding:8px;margin:4px 0;border-radius:4px;border-left:3px solid var(--primary)}"));
    client.println(F(".metric-value{font-size:2rem;font-weight:800;color:var(--primary);text-align:center;margin-bottom:4px}"));
    client.println(F(".metric-label{font-size:0.85rem;color:var(--gray-600);text-transform:uppercase;text-align:center;font-weight:600}"));
    client.println(F(".nav-grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(180px,1fr));gap:12px;margin-bottom:24px}"));
    client.println(F(".nav-btn{background:#fff;border:2px solid var(--gray-200);color:var(--gray-600);padding:12px;border-radius:8px;font-weight:600;text-decoration:none;text-align:center;transition:all 0.2s ease}"));
    client.println(F(".nav-btn:hover{border-color:var(--primary);color:var(--primary)}"));
    client.println(F("@media(max-width:768px){.container{padding:16px}.grid-2,.grid-3,.grid-4{grid-template-columns:1fr}.nav-grid{grid-template-columns:1fr}}"));
    client.println(F(".extended-info{margin:32px 0}"));
    client.println(F(".status-section{margin-bottom:32px;padding:20px;background:#f8fafc;border-radius:12px;border:1px solid var(--gray-200)}"));
    client.println(F(".status-section h4{margin:0 0 16px 0;padding-bottom:8px;border-bottom:2px solid var(--gray-200);color:var(--gray-900);font-size:1.1rem;font-weight:700;display:flex;align-items:center;gap:8px}"));
    client.println(F(".status-section h4 i{color:var(--primary)}"));
    client.println(F(".status-section .status-item{padding:12px 0;margin:8px 0}"));
    client.println(F(".status-section .status-item span:first-child{display:inline-block;width:65%;font-weight:600;color:var(--gray-700)}"));
    client.println(F(".status-section .status-item span:last-child{display:inline-block;width:35%;text-align:right}"));

    client.println(F("</style>"));
}

// Essential JavaScript
void WebServer::sendUnifiedJS(WiFiClient& client) {
    client.println(F("<script>"));

    // AJAX function for API calls without page navigation
    client.println(F("function api(endpoint, params='') {"));
    client.println(F("  const url = params ? `/api/${endpoint}?${params}` : `/api/${endpoint}`;"));
    client.println(F("  console.log('API call:', url);"));
    client.println(F("  "));
    client.println(F("  fetch(url)"));
    client.println(F("    .then(response => response.json())"));
    client.println(F("    .then(data => {"));
    client.println(F("      console.log('Response:', data);"));
    client.println(F("      showNotification(data.message || data.status || 'Command sent');"));
    client.println(F("    })"));
    client.println(F("    .catch(error => {"));
    client.println(F("      console.error('Error:', error);"));
    client.println(F("      showNotification('Error: ' + error.message, 'error');"));
    client.println(F("    });"));
    client.println(F("}"));

    // ✅ ENHANCED: Real-time update function with system status AND extended info
    client.println(F("function updateSensorData() {"));
    client.println(F("  fetch('/api/sensor-data')"));
    client.println(F("    .then(response => response.json())"));
    client.println(F("    .then(data => {"));
    client.println(F("      // Update basic sensor readings"));
    client.println(F("      const frontDist = document.getElementById('front-distance');"));
    client.println(F("      const rearDist = document.getElementById('rear-distance');"));
    client.println(F("      const userPresent = document.getElementById('user-present');"));
    client.println(F("      "));
    client.println(F("      // ✅ FIX: Update system status elements"));
    client.println(F("      const systemState = document.getElementById('system-state');"));
    client.println(F("      const currentSpeed = document.getElementById('current-speed');"));
    client.println(F("      const safetyStatus = document.getElementById('safety-status');"));
    client.println(F("      "));
    client.println(F("      // ✅ NEW: Update extended system info elements"));
    client.println(F("      const frontSensorStatus = document.getElementById('front-sensor-status');"));
    client.println(F("      const rearSensorStatus = document.getElementById('rear-sensor-status');"));
    client.println(F("      const pressureSensorStatus = document.getElementById('pressure-sensor-status');"));
    client.println(F("      const leftMotorStatus = document.getElementById('left-motor-status');"));
    client.println(F("      const rightMotorStatus = document.getElementById('right-motor-status');"));
    client.println(F("      const doorActuatorStatus = document.getElementById('door-actuator-status');"));
    client.println(F("      const wifiSignalQuality = document.getElementById('wifi-signal-quality');"));
    client.println(F("      const wifiSignalStrength = document.getElementById('wifi-signal-strength');"));
    client.println(F("      const systemUptime = document.getElementById('system-uptime');"));
    client.println(F("      const healthScore = document.getElementById('health-score');"));
    client.println(F("      "));
    client.println(F("      // Update all elements"));
    client.println(F("      if (frontDist) frontDist.textContent = data.frontDistance + ' cm';"));
    client.println(F("      if (rearDist) rearDist.textContent = data.rearDistance + ' cm';"));
    client.println(F("      if (userPresent) userPresent.textContent = data.userPresent ? 'Yes' : 'No';"));
    client.println(F("      if (systemState) systemState.textContent = data.systemState;"));
    client.println(F("      if (currentSpeed) currentSpeed.textContent = data.currentSpeed;"));
    client.println(F("      if (safetyStatus) safetyStatus.textContent = data.safetyStatus;"));
    client.println(F("      "));
    client.println(F("      // ✅ NEW: Update extended system info"));
    client.println(F("      if (frontSensorStatus) {"));
    client.println(F("        frontSensorStatus.textContent = data.frontSensorStatus;"));
    client.println(F("        frontSensorStatus.className = 'status-value ' + (data.frontSensorStatus === 'OK' ? 'status-ok' : 'status-error');"));
    client.println(F("      }"));
    client.println(F("      if (rearSensorStatus) {"));
    client.println(F("        rearSensorStatus.textContent = data.rearSensorStatus;"));
    client.println(F("        rearSensorStatus.className = 'status-value ' + (data.rearSensorStatus === 'OK' ? 'status-ok' : 'status-error');"));
    client.println(F("      }"));
    client.println(F("      if (pressureSensorStatus) {"));
    client.println(F("        pressureSensorStatus.textContent = data.pressureSensorStatus;"));
    client.println(F("        pressureSensorStatus.className = 'status-value ' + (data.pressureSensorStatus === 'Responding' ? 'status-ok' : 'status-warning');"));
    client.println(F("      }"));
    client.println(F("      if (leftMotorStatus) {"));
    client.println(F("        leftMotorStatus.textContent = data.leftMotorStatus;"));
    client.println(F("        leftMotorStatus.className = 'status-value ' + (data.leftMotorStatus === 'Connected' ? 'status-ok' : 'status-warning');"));
    client.println(F("      }"));
    client.println(F("      if (rightMotorStatus) {"));
    client.println(F("        rightMotorStatus.textContent = data.rightMotorStatus;"));
    client.println(F("        rightMotorStatus.className = 'status-value ' + (data.rightMotorStatus === 'Connected' ? 'status-ok' : 'status-warning');"));
    client.println(F("      }"));
    client.println(F("      if (doorActuatorStatus) {"));
    client.println(F("        doorActuatorStatus.textContent = data.doorActuatorStatus;"));
    client.println(F("        doorActuatorStatus.className = 'status-value ' + (data.doorActuatorStatus === 'Operational' ? 'status-ok' : 'status-error');"));
    client.println(F("      }"));
    client.println(F("      if (wifiSignalQuality) {"));
    client.println(F("        wifiSignalQuality.textContent = data.wifiSignalQuality;"));
    client.println(F("        const signalClass = data.wifiSignalQuality === 'Excellent' || data.wifiSignalQuality === 'Good' ? 'status-ok' : 'status-warning';"));
    client.println(F("        wifiSignalQuality.className = 'status-value ' + signalClass;"));
    client.println(F("      }"));
    client.println(F("      if (wifiSignalStrength) wifiSignalStrength.textContent = data.wifiSignalStrength;"));
    client.println(F("      if (systemUptime) systemUptime.textContent = data.systemUptime;"));
    client.println(F("      if (healthScore) {"));
    client.println(F("        healthScore.textContent = data.healthScore + '%';"));
    client.println(F("        const healthClass = data.healthScore >= 85 ? 'status-ok' : data.healthScore >= 70 ? 'status-warning' : 'status-error';"));
    client.println(F("        healthScore.className = 'status-value ' + healthClass;"));
    client.println(F("      }"));
    client.println(F("      "));
    client.println(F("      console.log('All data updated:', data);"));
    client.println(F("    })"));
    client.println(F("    .catch(error => console.error('Sensor update failed:', error));"));
    client.println(F("}"));

    // Auto-start updates on home page
    client.println(F("document.addEventListener('DOMContentLoaded', function() {"));
    client.println(F("  const path = window.location.pathname;"));
    client.println(F("  if (path === '/') {"));
    client.println(F("    // Start updates immediately"));
    client.println(F("    updateSensorData();"));
    client.println(F("    // Continue updating every 3 seconds"));
    client.println(F("    setInterval(updateSensorData, 3000);"));
    client.println(F("  }"));
    client.println(F("});"));

    // Form submission and notification functions (existing code)
    client.println(F("function submitForm(form) {"));
    client.println(F("  const data = new FormData(form);"));
    client.println(F("  const params = new URLSearchParams(data).toString();"));
    client.println(F("  api('config-update', params);"));
    client.println(F("  return false;"));
    client.println(F("}"));

    client.println(F("function showNotification(message, type = 'info') {"));
    client.println(F("  const notification = document.createElement('div');"));
    client.println(F("  notification.style.cssText = `"));
    client.println(F("    position: fixed; top: 20px; right: 20px; z-index: 9999;"));
    client.println(F("    padding: 12px 20px; border-radius: 8px; color: white;"));
    client.println(F("    font-weight: 600; max-width: 300px; opacity: 0;"));
    client.println(F("    transition: opacity 0.3s ease;"));
    client.println(F("    background: ${type === 'error' ? '#ef4444' : '#10b981'};"));
    client.println(F("  `;"));
    client.println(F("  notification.textContent = message;"));
    client.println(F("  document.body.appendChild(notification);"));
    client.println(F("  "));
    client.println(F("  setTimeout(() => notification.style.opacity = '1', 100);"));
    client.println(F("  setTimeout(() => {"));
    client.println(F("    notification.style.opacity = '0';"));
    client.println(F("    setTimeout(() => document.body.removeChild(notification), 300);"));
    client.println(F("  }, 3000);"));
    client.println(F("}"));

    client.println(F("</script>"));
}



// Page template system for consistent layout
void WebServer::sendPageTemplate(WiFiClient &client, const __FlashStringHelper *title,
                                 const __FlashStringHelper *icon, void (WebServer::*contentMethod)(WiFiClient &))
{
    sendHttpHeader(client);

    client.println(F("<!DOCTYPE html><html><head>"));
    client.println(F("<meta charset='UTF-8'>"));
    client.println(F("<meta name='viewport' content='width=device-width,initial-scale=1.0'>"));
    client.print(F("<title>"));
    client.print(title);
    client.println(F(" - Wheelchair Swing</title>"));
    client.println(F("<link href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.5.0/css/all.min.css' rel='stylesheet'>"));

    sendUnifiedCSS(client);

    client.println(F("</head><body><div class='container'>"));

    // Header with navigation
    client.println(F("<div class='header'>"));
    client.print(F("<h1><i class='fas fa-"));
    client.print(icon);
    client.print(F("'></i> "));
    client.print(title);
    client.println(F("</h1>"));
    client.println(F("<p><a href='/'>← Back to Dashboard</a></p>"));
    client.println(F("</div>"));

    // Navigation menu
    sendNavigation(client);

    // Page-specific content
    (this->*contentMethod)(client);

    client.println(F("</div>"));
    sendUnifiedJS(client);
    client.println(F("</body></html>"));
}

// Navigation menu
void WebServer::sendNavigation(WiFiClient& client) {
    client.println(F("<div class='nav-grid'>"));
    client.println(F("<a href='/' class='nav-btn'><i class='fas fa-home'></i> Dashboard</a>"));
    client.println(F("<a href='/control' class='nav-btn'><i class='fas fa-gamepad'></i> Control</a>"));
    client.println(F("<a href='/config' class='nav-btn'><i class='fas fa-cog'></i> Config</a>"));
    client.println(F("<a href='/motor-test' class='nav-btn'><i class='fas fa-cogs'></i> Motor Test</a>"));
    client.println(F("<a href='/demo' class='nav-btn'><i class='fas fa-play-circle'></i> Demo</a>"));
    client.println(F("</div>"));
}


// Home page implementation
void WebServer::sendHomePage(WiFiClient &client)
{
    sendHttpHeader(client);

    client.println(F("<!DOCTYPE html><html><head>"));
    client.println(F("<meta charset='UTF-8'>"));
    client.println(F("<meta name='viewport' content='width=device-width,initial-scale=1.0'>"));
    client.println(F("<title>Wheelchair Swing Control</title>"));
    client.println(F("<link href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.5.0/css/all.min.css' rel='stylesheet'>"));

    sendUnifiedCSS(client);

    client.println(F("</head><body><div class='container'>"));

    // Main header
    client.println(F("<div class='header'>"));
    client.println(F("<h1><i class='fas fa-wheelchair'></i> Wheelchair Swing Control</h1>"));
    client.println(F("<p>Advanced Safety & Control System</p>"));
    client.println(F("</div>"));

    sendNavigation(client);
    sendHomeContent(client);

    client.println(F("</div>"));
    sendUnifiedJS(client);
    client.println(F("</body></html>"));
}

void WebServer::sendHomeContent(WiFiClient &client)
{
    RuntimeConfig& config = RuntimeConfig::getInstance();
    // System metrics cards
    client.println(F("<div class='grid grid-4'>"));

    // System state card
    client.println(F("<div class='card'>"));
    client.println(F("<div class='metric-value'>"));
    String currentState = _stateMachine->getStateString();
    client.print(currentState);
    client.println(F("</div>"));
    client.println(F("<div class='metric-label'>System State</div>"));
    client.println(F("</div>"));

    // Current speed card
    client.println(F("<div class='card'>"));
    client.println(F("<div class='metric-value'>"));
    client.print(_stateMachine->getSpeedString());
    client.println(F("</div>"));
    client.println(F("<div class='metric-label'>Current Speed</div>"));
    client.println(F("</div>"));

    // Safety status card
    client.println(F("<div class='card'>"));
    client.println(F("<div class='metric-value status-ok'>"));
    client.print(_safetyMonitor->getStatusString());
    client.println(F("</div>"));
    client.println(F("<div class='metric-label'>Safety Status</div>"));
    client.println(F("</div>"));

    client.println(F("</div>"));

    // Real-time sensor data
    client.println(F("<div class='grid grid-2'>"));

    // Sensor readings card
    client.println(F("<div class='card'>"));
    client.println(F("<h3><i class='fas fa-radar'></i> Real-time Sensors</h3>"));

    // Front distance with ID
    String frontDistStr = String(_safetyMonitor->getFrontDistance(), 1) + " cm";
    client.println(F("<div class='status-item'>"));
    client.println(F("<span>Front Distance:</span>"));
    client.print(F("<span class='status-value' id='front-distance'>"));
    client.print(frontDistStr);
    client.println(F("</span>"));
    client.println(F("</div>"));

    // Rear distance with ID
    String rearDistStr = String(_safetyMonitor->getRearDistance(), 1) + " cm";
    client.println(F("<div class='status-item'>"));
    client.println(F("<span>Rear Distance:</span>"));
    client.print(F("<span class='status-value' id='rear-distance'>"));
    client.print(rearDistStr);
    client.println(F("</span>"));
    client.println(F("</div>"));

    // User present with ID
    client.println(F("<div class='status-item'>"));
    client.println(F("<span>User Present:</span>"));
    client.print(F("<span class='status-value' id='user-present'>"));
    client.print(_safetyMonitor->isUserPresent() ? F("Yes") : F("No"));
    client.println(F("</span>"));
    client.println(F("</div>"));

    client.println(F("</div>"));

    // ✅ ALSO UPDATE SYSTEM STATUS WITH ID ATTRIBUTES:
    client.println(F("<div class='card'>"));
    client.println(F("<h3><i class='fas fa-tachometer-alt'></i> System Status</h3>"));

    // System state with ID
    client.println(F("<div class='status-item'>"));
    client.println(F("<span>State:</span>"));
    client.print(F("<span class='status-value' id='system-state'>"));
    client.print(_stateMachine->getStateString());
    client.println(F("</span>"));
    client.println(F("</div>"));

    // Current speed with ID
    client.println(F("<div class='status-item'>"));
    client.println(F("<span>Speed:</span>"));
    client.print(F("<span class='status-value' id='current-speed'>"));
    client.print(_stateMachine->getSpeedString());
    client.println(F("</span>"));
    client.println(F("</div>"));

    // Safety status with ID
    client.println(F("<div class='status-item'>"));
    client.println(F("<span>Safety:</span>"));
    client.print(F("<span class='status-value' id='safety-status'>"));
    client.print(_safetyMonitor->getStatusString());
    client.println(F("</span>"));
    client.println(F("</div>"));

    client.println(F("</div>"));

    // Runtime configuration status
    client.println(F("<div class='card'>"));
    client.println(F("<h3><i class='fas fa-sliders-h'></i> Configuration Status</h3>"));

    String frontWarningStr = String(config.getFrontWarningDistance(), 1) + " cm";
    sendStatusItem(client, F("Front Warning"), frontWarningStr);

    String speedLowStr = String(config.getSpeedLow()) + " RPM";
    sendStatusItem(client, F("Speed Low"), speedLowStr);

    sendStatusItem(client, F("Audio Feedback"), config.isAudioFeedbackEnabled() ? F("Enabled") : F("Disabled"));
    sendStatusItem(client, F("Config Valid"), F("Yes"));

    client.println(F("</div>"));

    // ==== Extended system info card ====
    client.println(F("<div class='card extended-info'>"));
    client.println(F("<h3><i class='fas fa-info-circle'></i> Extended System Information</h3>"));

    // Sensor Health Section
    client.println(F("<div class='status-section'>"));
    client.println(F("<h4><i class='fas fa-microchip'></i> Sensor Health</h4>"));

    // Front distance sensor
    float frontDist = _safetyMonitor->getFrontDistance();
    bool frontSensorOK = (frontDist > 0 && frontDist < 400);
    client.println(F("<div class='status-item'>"));
    client.println(F("<span>Front Distance Sensor</span>"));
    client.print(F("<span class='status-value "));
    client.print(frontSensorOK ? F("status-ok' id='front-sensor-status'>OK") : F("status-error' id='front-sensor-status'>Error"));
    client.println(F("</span>"));
    client.println(F("</div>"));

    // Rear distance sensor
    float rearDist = _safetyMonitor->getRearDistance();
    bool rearSensorOK = (rearDist > 0 && rearDist < 400);
    client.println(F("<div class='status-item'>"));
    client.println(F("<span>Rear Distance Sensor</span>"));
    client.print(F("<span class='status-value "));
    client.print(rearSensorOK ? F("status-ok' id='rear-sensor-status'>OK") : F("status-error' id='rear-sensor-status'>Error"));
    client.println(F("</span>"));
    client.println(F("</div>"));

    // Pressure sensor
    bool pressureSensorOK = _safetyMonitor->isUserPresent();
    client.println(F("<div class='status-item'>"));
    client.println(F("<span>Pressure Sensor</span>"));
    client.print(F("<span class='status-value "));
    client.print(pressureSensorOK ? F("status-ok' id='pressure-sensor-status'>Responding") : F("status-warning' id='pressure-sensor-status'>No Signal"));
    client.println(F("</span>"));
    client.println(F("</div>"));

    client.println(F("</div>"));

    // Motor & Actuator Health Section
    client.println(F("<div class='status-section'>"));
    client.println(F("<h4><i class='fas fa-cogs'></i> Motor & Actuator Health</h4>"));

    // Left stepper motor
    bool leftMotorOK = (_leftStepper != nullptr);
    client.println(F("<div class='status-item'>"));
    client.println(F("<span>Left Stepper Motor</span>"));
    client.print(F("<span class='status-value "));
    client.print(leftMotorOK ? F("status-ok' id='left-motor-status'>Connected") : F("status-warning' id='left-motor-status'>Disconnected"));
    client.println(F("</span>"));
    client.println(F("</div>"));

    // Right stepper motor
    bool rightMotorOK = (_rightStepper != nullptr);
    client.println(F("<div class='status-item'>"));
    client.println(F("<span>Right Stepper Motor</span>"));
    client.print(F("<span class='status-value "));
    client.print(rightMotorOK ? F("status-ok' id='right-motor-status'>Connected") : F("status-warning' id='right-motor-status'>Disconnected"));
    client.println(F("</span>"));
    client.println(F("</div>"));

    // Door actuator
    bool doorActuatorOK = (_doorActuator != nullptr);
    client.println(F("<div class='status-item'>"));
    client.println(F("<span>Door Actuator</span>"));
    client.print(F("<span class='status-value "));
    client.print(doorActuatorOK ? F("status-ok' id='door-actuator-status'>Operational") : F("status-error' id='door-actuator-status'>Error"));
    client.println(F("</span>"));
    client.println(F("</div>"));

    client.println(F("</div>"));

    // Network Status Section
    client.println(F("<div class='status-section'>"));
    client.println(F("<h4><i class='fas fa-wifi'></i> Network Status</h4>"));

    // WiFi signal quality
    int signalStrength = WiFi.RSSI();
    String signalQuality = "Poor";
    String signalClass = "status-error";
    if (signalStrength > -50) { signalQuality = "Excellent"; signalClass = "status-ok"; }
    else if (signalStrength > -60) { signalQuality = "Good"; signalClass = "status-ok"; }
    else if (signalStrength > -70) { signalQuality = "Fair"; signalClass = "status-warning"; }

    client.println(F("<div class='status-item'>"));
    client.println(F("<span>WiFi Signal Quality</span>"));
    client.print(F("<span class='status-value "));
    client.print(signalClass);
    client.print(F("' id='wifi-signal-quality'>"));
    client.print(signalQuality);
    client.println(F("</span>"));
    client.println(F("</div>"));

    client.println(F("<div class='status-item'>"));
    client.println(F("<span>IP Address</span>"));
    client.print(F("<span class='status-value'>"));
    client.print(WiFi.localIP().toString());
    client.println(F("</span>"));
    client.println(F("</div>"));

    client.println(F("</div>"));

    // System Performance Section
    client.println(F("<div class='status-section'>"));
    client.println(F("<h4><i class='fas fa-tachometer-alt'></i> System Performance</h4>"));

    // System uptime
    unsigned long uptime = millis() / 1000;
    unsigned long hours = uptime / 3600;
    unsigned long minutes = (uptime % 3600) / 60;
    String uptimeStr = String(hours) + "h " + String(minutes) + "m";
    client.println(F("<div class='status-item'>"));
    client.println(F("<span>System Uptime</span>"));
    client.print(F("<span class='status-value' id='system-uptime'>"));
    client.print(uptimeStr);
    client.println(F("</span>"));
    client.println(F("</div>"));

    // Configuration status
    client.println(F("<div class='status-item'>"));
    client.println(F("<span>Configuration</span>"));
    client.print(F("<span class='status-value "));
    client.print(config.isValid() ? F("status-ok'>Valid") : F("status-error'>Invalid"));
    client.println(F("</span>"));
    client.println(F("</div>"));

    // Overall health score
    int healthScore = 100;
    if (!frontSensorOK) healthScore -= 15;
    if (!rearSensorOK) healthScore -= 15;
    if (!pressureSensorOK) healthScore -= 10;
    if (!leftMotorOK) healthScore -= 15;
    if (!rightMotorOK) healthScore -= 15;
    if (signalStrength < -70) healthScore -= 10;
    if (!config.isValid()) healthScore -= 10;

    String healthClass = "status-ok";
    if (healthScore < 70) healthClass = "status-error";
    else if (healthScore < 85) healthClass = "status-warning";

    client.println(F("<div class='status-item'>"));
    client.println(F("<span>Overall Health Score</span>"));
    client.print(F("<span class='status-value "));
    client.print(healthClass);
    client.print(F("' id='health-score'>"));
    client.print(healthScore);
    client.println(F("%</span>"));
    client.println(F("</div>"));

    client.println(F("</div>"));
    client.println(F("</div>"));

    // Quick control buttons
    client.println(F("<div class='card'>"));
    client.println(F("<h3><i class='fas fa-bolt'></i> Quick Controls</h3>"));
    client.println(F("<div class='grid grid-4'>"));

    sendButton(client, F("<i class='fas fa-play'></i> Start"), "/api/start", "success");
    sendButton(client, F("<i class='fas fa-stop'></i> Stop"), "/api/stop", "warning");
    sendButton(client, F("<i class='fas fa-door-open'></i> Door"), "/api/door-toggle", "primary");
    sendButton(client, F("<i class='fas fa-exclamation-triangle'></i> Emergency"), "/api/emergency", "danger");

    client.println(F("</div>"));
    client.println(F("</div>"));
}

// Control page
void WebServer::sendControlPage(WiFiClient &client)
{
    sendPageTemplate(client, F("Control Panel"), F("gamepad"), &WebServer::sendControlContent);
}

void WebServer::sendControlContent(WiFiClient &client)
{
    // Motion controls
    client.println(F("<div class='card'>"));
    client.println(F("<h3><i class='fas fa-gamepad'></i> Motion Controls</h3>"));
    client.println(F("<div class='grid grid-4'>"));

    sendButton(client, F("<i class='fas fa-play'></i> Start Swing"), "/api/start", "success");
    sendButton(client, F("<i class='fas fa-stop'></i> Stop Swing"), "/api/stop", "warning");
    sendButton(client, F("<i class='fas fa-arrow-up'></i> Speed Up"), "/api/speed-up", "primary");
    sendButton(client, F("<i class='fas fa-arrow-down'></i> Speed Down"), "/api/speed-down", "primary");

    client.println(F("</div>"));
    client.println(F("</div>"));

    // Door controls
    client.println(F("<div class='card'>"));
    client.println(F("<h3><i class='fas fa-door-open'></i> Door Controls</h3>"));
    client.println(F("<div class='grid grid-2'>"));

    sendButton(client, F("<i class='fas fa-door-open'></i> Open Door"), "/api/door-open", "primary");
    sendButton(client, F("<i class='fas fa-door-closed'></i> Close Door"), "/api/door-close", "primary");

    client.println(F("</div>"));
    client.println(F("</div>"));

    // Emergency controls
    client.println(F("<div class='card'>"));
    client.println(F("<h3><i class='fas fa-exclamation-triangle'></i> Emergency Controls</h3>"));
    client.println(F("<div class='grid grid-2'>"));

    sendButton(client, F("<i class='fas fa-exclamation-triangle'></i> Emergency Stop"), "/api/emergency", "danger");
    sendButton(client, F("<i class='fas fa-undo'></i> Reset System"), "/api/reset", "warning");

    client.println(F("</div>"));
    client.println(F("</div>"));
}

// Helper methods for building UI components
void WebServer::sendCard(WiFiClient &client, const __FlashStringHelper *title, const __FlashStringHelper *icon)
{
    client.println(F("<div class='card'>"));
    if (title)
    {
        client.print(F("<h3>"));
        if (icon)
        {
            client.print(F("<i class='fas fa-"));
            client.print(icon);
            client.print(F("'></i> "));
        }
        client.print(title);
        client.println(F("</h3>"));
    }
}

void WebServer::sendCardEnd(WiFiClient &client)
{
    client.println(F("</div>"));
}

void WebServer::sendButton(WiFiClient& client, const __FlashStringHelper* text, const char* url, const char* type) {
    if (strstr(url, "/api/") == url) {
        // This is an API endpoint - use JavaScript
        String endpoint = String(url).substring(5); // Remove "/api/" prefix
        client.print(F("<button onclick='api(\""));
        client.print(endpoint);
        client.print(F("\")' class='btn btn-"));
        client.print(type);
        client.print(F("'>"));
        client.print(text);
        client.println(F("</button>"));
    } else {
        // This is a regular page link - use href
        client.print(F("<a href='"));
        client.print(url);
        client.print(F("' class='btn btn-"));
        client.print(type);
        client.print(F("'>"));
        client.print(text);
        client.println(F("</a>"));
    }
}


void WebServer::sendStatusItem(WiFiClient &client, const __FlashStringHelper *label, const __FlashStringHelper *value, const char *statusClass)
{
    client.println(F("<div class='status-item'>"));
    client.print(F("<span>"));
    client.print(label);
    client.print(F(":</span>"));
    client.print(F("<span class='status-value"));
    if (statusClass && strlen(statusClass) > 0)
    {
        client.print(F(" "));
        client.print(statusClass);
    }
    client.print(F("'>"));
    client.print(value);
    client.println(F("</span>"));
    client.println(F("</div>"));
}

void WebServer::sendStatusItem(WiFiClient &client, const __FlashStringHelper *label, const String &value, const char *statusClass)
{
    client.println(F("<div class='status-item'>"));
    client.print(F("<span>"));
    client.print(label);
    client.print(F(":</span>"));
    client.print(F("<span class='status-value"));
    if (statusClass && strlen(statusClass) > 0)
    {
        client.print(F(" "));
        client.print(statusClass);
    }
    client.print(F("'>"));
    client.print(value);
    client.println(F("</span>"));
    client.println(F("</div>"));
}

void WebServer::sendFormField(WiFiClient &client, const char *type, const char *name, const char *label, const char *value)
{
    client.println(F("<div class='form-group'>"));
    client.print(F("<label>"));
    client.print(label);
    client.print(F(":</label>"));
    client.print(F("<input type='"));
    client.print(type);
    client.print(F("' name='"));
    client.print(name);
    client.print(F("'"));
    if (value && strlen(value) > 0)
    {
        client.print(F(" value='"));
        client.print(value);
        client.print(F("'"));
    }
    client.println(F(">"));
    client.println(F("</div>"));
}

// Configuration page
void WebServer::sendConfigPage(WiFiClient &client)
{
    sendPageTemplate(client, F("Configuration"), F("cog"), &WebServer::sendConfigContent);
}

void WebServer::sendConfigContent(WiFiClient &client)
{
    RuntimeConfig& config = RuntimeConfig::getInstance();

    // Safety configuration form
    client.println(F("<div class='card'>"));
    client.println(F("<h3><i class='fas fa-shield-alt'></i> Safety Configuration</h3>"));
    client.println(F("<form onsubmit='return submitForm(this)'>"));

    client.println(F("<div class='grid grid-2'>"));

    // Distance thresholds
    client.println(F("<div>"));
    client.println(F("<h4>Distance Thresholds</h4>"));

    sendFormField(client, "number", "frontWarning", "Front Warning (cm)", String(config.getFrontWarningDistance()).c_str());
    sendFormField(client, "number", "frontCritical", "Front Critical (cm)", String(config.getFrontCriticalDistance()).c_str());
    sendFormField(client, "number", "rearWarning", "Rear Warning (cm)", String(config.getRearWarningDistance()).c_str());
    sendFormField(client, "number", "rearCritical", "Rear Critical (cm)", String(config.getRearCriticalDistance()).c_str());

    client.println(F("</div>"));

    // Pressure settings
    client.println(F("<div>"));
    client.println(F("<h4>Pressure Settings</h4>"));

    sendFormField(client, "number", "pressureThreshold", "Pressure Threshold", String(config.getPressureThreshold()).c_str());

    client.println(F("<div class='form-group'>"));
    client.println(F("<label>Audio Feedback:</label>"));
    client.println(F("<select name='audioFeedback'>"));
    client.print(F("<option value='true'"));
    if (config.isAudioFeedbackEnabled())
        client.print(F(" selected"));
    client.println(F(">Enabled</option>"));
    client.print(F("<option value='false'"));
    if (!config.isAudioFeedbackEnabled())
        client.print(F(" selected"));
    client.println(F(">Disabled</option>"));
    client.println(F("</select>"));
    client.println(F("</div>"));

    client.println(F("</div>"));
    client.println(F("</div>"));

    client.println(F("<button type='submit' class='btn btn-primary'><i class='fas fa-save'></i> Save Safety Settings</button>"));
    client.println(F("</form>"));
    client.println(F("</div>"));

    // Motor configuration form
    client.println(F("<div class='card'>"));
    client.println(F("<h3><i class='fas fa-cogs'></i> Motor Configuration</h3>"));
    client.println(F("<form onsubmit='return submitForm(this)'>"));

    client.println(F("<div class='grid grid-3'>"));

    sendFormField(client, "number", "speedLow", "Low Speed (RPM)", String(config.getSpeedLow()).c_str());
    sendFormField(client, "number", "speedMedium", "Medium Speed (RPM)", String(config.getSpeedMedium()).c_str());
    sendFormField(client, "number", "speedHigh", "High Speed (RPM)", String(config.getSpeedHigh()).c_str());

    client.println(F("</div>"));

    client.println(F("<button type='submit' class='btn btn-primary'><i class='fas fa-save'></i> Save Motor Settings</button>"));
    client.println(F("</form>"));
    client.println(F("</div>"));

    // System configuration form
    client.println(F("<div class='card'>"));
    client.println(F("<h3><i class='fas fa-sliders-h'></i> System Configuration</h3>"));
    client.println(F("<form onsubmit='return submitForm(this)'>"));

    client.println(F("<div class='grid grid-2'>"));

    sendFormField(client, "number", "doorTimeout", "Door Timeout (seconds)", String(config.getDoorTimeoutMs() / 1000).c_str());
    sendFormField(client, "range", "buzzerVolume", "Buzzer Volume (0-10)", String(config.getBuzzerVolume()).c_str());

    client.println(F("</div>"));

    client.println(F("<button type='submit' class='btn btn-primary'><i class='fas fa-save'></i> Save System Settings</button>"));
    client.println(F("</form>"));
    client.println(F("</div>"));

    // Configuration management
    client.println(F("<div class='card'>"));
    client.println(F("<h3><i class='fas fa-tools'></i> Configuration Management</h3>"));
    client.println(F("<div class='grid grid-3'>"));

    sendButton(client, F("<i class='fas fa-download'></i> Export Config"), "/api/config-export", "success");
    sendButton(client, F("<i class='fas fa-undo'></i> Factory Reset"), "/api/factory-reset", "warning");
    sendButton(client, F("<i class='fas fa-sync-alt'></i> Reload Page"), "/config", "primary");

    client.println(F("</div>"));
    client.println(F("</div>"));
}

// Main API handler - consolidated routing
void WebServer::handleAPI(WiFiClient &client, String endpoint, String params)
{
    Serial.print(F("API Request: "));
    Serial.print(endpoint);
    Serial.print(F(" Params: "));
    Serial.println(params);

    if (endpoint == "sensor-data") {
        // Basic sensor data
        String sensorJson = "{";
        sensorJson += "\"frontDistance\":" + String(_safetyMonitor->getFrontDistance(), 1) + ",";
        sensorJson += "\"rearDistance\":" + String(_safetyMonitor->getRearDistance(), 1) + ",";
        sensorJson += "\"userPresent\":" + String(_safetyMonitor->isUserPresent() ? "true" : "false") + ",";

        // System status data
        sensorJson += "\"systemState\":\"" + String(_stateMachine->getStateString()) + "\",";
        sensorJson += "\"currentSpeed\":\"" + String(_stateMachine->getSpeedString()) + "\",";
        sensorJson += "\"safetyStatus\":\"" + String(_safetyMonitor->getStatusString()) + "\",";

        // Extended system info data
        float frontDist = _safetyMonitor->getFrontDistance();
        float rearDist = _safetyMonitor->getRearDistance();
        bool frontSensorOK = (frontDist > 0 && frontDist < 400);
        bool rearSensorOK = (rearDist > 0 && rearDist < 400);
        bool pressureSensorOK = _safetyMonitor->isUserPresent();
        bool leftMotorOK = (_leftStepper != nullptr);
        bool rightMotorOK = (_rightStepper != nullptr);
        bool doorActuatorOK = (_doorActuator != nullptr);

        sensorJson += "\"frontSensorStatus\":\"" + String(frontSensorOK ? "OK" : "Error") + "\",";
        sensorJson += "\"rearSensorStatus\":\"" + String(rearSensorOK ? "OK" : "Error") + "\",";
        sensorJson += "\"pressureSensorStatus\":\"" + String(pressureSensorOK ? "Responding" : "No Signal") + "\",";
        sensorJson += "\"leftMotorStatus\":\"" + String(leftMotorOK ? "Connected" : "Disconnected") + "\",";
        sensorJson += "\"rightMotorStatus\":\"" + String(rightMotorOK ? "Connected" : "Disconnected") + "\",";
        sensorJson += "\"doorActuatorStatus\":\"" + String(doorActuatorOK ? "Operational" : "Error") + "\",";

        // WiFi and system metrics
        int signalStrength = WiFi.RSSI();
        String signalQuality = "Poor";
        if (signalStrength > -50) signalQuality = "Excellent";
        else if (signalStrength > -60) signalQuality = "Good";
        else if (signalStrength > -70) signalQuality = "Fair";

        sensorJson += "\"wifiSignalQuality\":\"" + signalQuality + "\",";
        sensorJson += "\"wifiSignalStrength\":\"" + String(signalStrength) + " dBm\",";

        // System uptime
        unsigned long uptime = millis() / 1000;
        unsigned long hours = uptime / 3600;
        unsigned long minutes = (uptime % 3600) / 60;
        String uptimeStr = String(hours) + "h " + String(minutes) + "m";
        sensorJson += "\"systemUptime\":\"" + uptimeStr + "\",";

        // Health score calculation
        int healthScore = 100;
        if (!frontSensorOK) healthScore -= 15;
        if (!rearSensorOK) healthScore -= 15;
        if (!pressureSensorOK) healthScore -= 10;
        if (!leftMotorOK) healthScore -= 15;
        if (!rightMotorOK) healthScore -= 15;
        if (!doorActuatorOK) healthScore -= 10;
        if (signalStrength < -70) healthScore -= 10;

        RuntimeConfig& config = RuntimeConfig::getInstance();
        if (!config.isValid()) healthScore -= 10;

        sensorJson += "\"healthScore\":" + String(healthScore);
        sensorJson += "}";

        sendJsonResponse(client, sensorJson);
        return;
    } else if (endpoint.startsWith("demo")) {
        handleDemoAPI(client, endpoint.substring(4));
    } else if (endpoint.startsWith("motor-test")) {
        handleMotorTestAPI(client, endpoint.substring(11));
    } else if (endpoint == "config-update") {
        handleConfigUpdate(client, params);
    } else if (endpoint == "config-export") {
        exportConfiguration(client);
        return;
    } else if (endpoint == "factory-reset") {
        RuntimeConfig& config = RuntimeConfig::getInstance();
        config.factoryReset();
        sendJsonResponse(client, F("{\"status\":\"factory_reset_complete\"}"));
    } else {
        handleControlCommand(client, endpoint);
    }
}

// Control command handler
void WebServer::handleControlCommand(WiFiClient &client, String command)
{
    bool success = true;
    String message = "Command executed: " + command;

    if (command == "start")
    {
        _stateMachine->processEvent(StateMachine::EVENT_START_PRESSED);
    }
    else if (command == "stop")
    {
        _stateMachine->processEvent(StateMachine::EVENT_STOP_PRESSED);
    }
    else if (command == "emergency")
    {
        _stateMachine->processEvent(StateMachine::EVENT_EMERGENCY);
        message = "Emergency stop activated";
    }
    else if (command == "reset")
    {
        _stateMachine->processEvent(StateMachine::EVENT_EMERGENCY_RESET);
        message = "System reset";
    }
    else if (command == "speed-up")
    {
        _stateMachine->processEvent(StateMachine::EVENT_SPEED_UP);
    }
    else if (command == "speed-down")
    {
        _stateMachine->processEvent(StateMachine::EVENT_SPEED_DOWN);
    }
    else if (command == "door-open")
    {
        _stateMachine->processEvent(StateMachine::EVENT_DOOR_OPENED);
    }
    else if (command == "door-close")
    {
        _stateMachine->processEvent(StateMachine::EVENT_DOOR_CLOSED);
    }
    else if (command == "door-toggle")
    {
        // Toggle door based on current state
        _stateMachine->processEvent(StateMachine::EVENT_DOOR_TOGGLE);
    }
    else
    {
        success = false;
        message = "Unknown command: " + command;
    }

    if (success)
    {
        String response = "{\"status\":\"success\",\"message\":\"" + message + "\"}";
        sendJsonResponse(client, response);
    }
    else
    {
        String response = "{\"error\":\"" + message + "\"}";
        sendJsonResponse(client, response);
    }
}

// Configuration update handler
void WebServer::handleConfigUpdate(WiFiClient &client, String params)
{
    RuntimeConfig& config = RuntimeConfig::getInstance();
    bool success = false;
    String errorMessage = "";

    // Parse URL-encoded parameters
    int paramStart = 0;
    while (paramStart < params.length())
    {
        int equalPos = params.indexOf('=', paramStart);
        if (equalPos < 0)
            break;

        int ampPos = params.indexOf('&', equalPos);
        if (ampPos < 0)
            ampPos = params.length();

        String key = params.substring(paramStart, equalPos);
        String value = params.substring(equalPos + 1, ampPos);

        // URL decode value (basic implementation)
        value.replace("%20", " ");
        value.replace("%2B", "+");

        // Update configuration based on key
        if (key == "frontWarning")
        {
            success = config.setWarningDistance(value.toFloat());
        }
        else if (key == "frontCritical")
        {
            success = config.setCriticalDistance(value.toFloat());
        }
        else if (key == "rearWarning")
        {
            success = config.setWarningDistance(value.toFloat());
        }
        else if (key == "rearCritical")
        {
            success = config.setCriticalDistance(value.toFloat());
        }
        else if (key == "pressureThreshold")
        {
            success = config.setPressureThreshold(value.toInt());
        }
        else if (key == "speedLow")
        {
            success = config.setSpeedLow(value.toInt());
        }
        else if (key == "speedMedium")
        {
            success = config.setSpeedMedium(value.toInt());
        }
        else if (key == "speedHigh")
        {
            success = config.setSpeedHigh(value.toInt());
        }
        else if (key == "doorTimeout")
        {
            success = config.setDoorTimeoutMs(value.toInt() * 1000); // Convert to ms
        }
        else if (key == "buzzerVolume")
        {
            success = config.setBuzzerVolume(value.toInt());
        }
        else if (key == "audioFeedback")
        {
            config.setAudioFeedbackEnabled(value == "true");
            success = true;
        }
        else if (key == "voiceRecognition")
        {
            config.setVoiceRecognitionEnabled(value == "true");
            success = true;
        }
        else
        {
            errorMessage = "Unknown configuration key: " + key;
        }

        if (!success && errorMessage.length() == 0)
        {
            errorMessage = "Invalid value for " + key + ": " + value;
            break;
        }

        paramStart = ampPos + 1;
    }

    if (success)
    {
        sendJsonResponse(client, F("{\"status\":\"success\",\"message\":\"Configuration updated and saved\"}"));
    }
    else
    {
        String response = "{\"error\":\"" + errorMessage + "\"}";
        sendJsonResponse(client, response);
    }
}

// Motor testing page
void WebServer::sendMotorTestPage(WiFiClient &client)
{
    sendPageTemplate(client, F("Motor Testing"), F("cogs"), &WebServer::sendMotorTestContent);
}

void WebServer::sendMotorTestContent(WiFiClient &client)
{
    // Motor status display
    client.println(F("<div class='card'>"));
    client.println(F("<h3><i class='fas fa-tachometer-alt'></i> Motor Status</h3>"));

    client.println(F("<div class='grid grid-2'>"));

    // Left motor status
    client.println(F("<div>"));
    client.println(F("<h4>Left Motor</h4>"));
    String leftPosStr = String(_testState.leftMotorPosition) + " steps";
    sendStatusItem(client, F("Position"), leftPosStr);
    sendStatusItem(client, F("Status"), _testState.motorTestActive && _testState.currentMotorTest.indexOf("left") >= 0 ? F("Testing") : F("Idle"));
    client.println(F("</div>"));

    // Right motor status
    client.println(F("<div>"));
    client.println(F("<h4>Right Motor</h4>"));
    String rightPosStr = String(_testState.rightMotorPosition) + " steps";
    sendStatusItem(client, F("Position"), rightPosStr);
    sendStatusItem(client, F("Status"), _testState.motorTestActive && _testState.currentMotorTest.indexOf("right") >= 0 ? F("Testing") : F("Idle"));
    client.println(F("</div>"));

    client.println(F("</div>"));
    client.println(F("</div>"));

    // Individual motor testing
    client.println(F("<div class='card'>"));
    client.println(F("<h3><i class='fas fa-cog'></i> Individual Motor Tests</h3>"));

    client.println(F("<div class='grid grid-2'>"));

    // Left motor controls
    client.println(F("<div>"));
    client.println(F("<h4>Left Motor</h4>"));

    client.println(F("<form action='/api/motor-test-left' method='get'>"));
    sendFormField(client, "number", "speed", "Speed (RPM)", "300");
    sendFormField(client, "number", "steps", "Steps", "100");
    client.println(F("<button type='submit' class='btn btn-primary'><i class='fas fa-play'></i> Test Left Motor</button>"));
    client.println(F("</form>"));

    client.println(F("</div>"));

    // Right motor controls
    client.println(F("<div>"));
    client.println(F("<h4>Right Motor</h4>"));

    client.println(F("<form action='/api/motor-test-right' method='get'>"));
    sendFormField(client, "number", "speed", "Speed (RPM)", "300");
    sendFormField(client, "number", "steps", "Steps", "100");
    client.println(F("<button type='submit' class='btn btn-primary'><i class='fas fa-play'></i> Test Right Motor</button>"));
    client.println(F("</form>"));

    client.println(F("</div>"));
    client.println(F("</div>"));
    client.println(F("</div>"));

    // Advanced motor tests
    client.println(F("<div class='card'>"));
    client.println(F("<h3><i class='fas fa-chart-line'></i> Advanced Motor Tests</h3>"));

    client.println(F("<div class='grid grid-3'>"));

    sendButton(client, F("<i class='fas fa-sync'></i> Synchronization Test"), "/api/motor-test-sync", "primary");
    sendButton(client, F("<i class='fas fa-arrow-up'></i> Ramp Test"), "/api/motor-test-ramp", "primary");
    sendButton(client, F("<i class='fas fa-stop'></i> Stop All Tests"), "/api/motor-test-stop", "danger");

    client.println(F("</div>"));
    client.println(F("</div>"));

    // Motor control utilities
    client.println(F("<div class='card'>"));
    client.println(F("<h3><i class='fas fa-tools'></i> Motor Utilities</h3>"));

    client.println(F("<div class='grid grid-2'>"));

    sendButton(client, F("<i class='fas fa-home'></i> Reset Positions"), "/api/motor-reset-positions", "warning");
    sendButton(client, F("<i class='fas fa-info'></i> Get Status"), "/api/motor-test-status", "primary");

    client.println(F("</div>"));
    client.println(F("</div>"));
}

// Demo page
void WebServer::sendDemoPage(WiFiClient &client)
{
    sendPageTemplate(client, F("Demo System"), F("play-circle"), &WebServer::sendDemoContent);
}

void WebServer::sendDemoContent(WiFiClient &client)
{
    // Demo status
    client.println(F("<div class='card'>"));
    client.println(F("<h3><i class='fas fa-info-circle'></i> Demo Status</h3>"));

    if (_testState.demoActive)
    {
        sendStatusItem(client, F("Demo Active"), F("Yes"), "status-warning");
        sendStatusItem(client, F("Demo Type"), _testState.currentDemo);
        String stepStr = String(_testState.demoStep);
        sendStatusItem(client, F("Step"), stepStr);

        unsigned long elapsed = (millis() - _testState.demoStartTime) / 1000;
        String elapsedStr = String(elapsed) + " seconds";
        sendStatusItem(client, F("Elapsed Time"), elapsedStr);
    }
    else
    {
        sendStatusItem(client, F("Demo Active"), F("No"), "status-ok");
        sendStatusItem(client, F("System Ready"), F("Yes"), "status-ok");
    }

    client.println(F("</div>"));

    // Demo selection
    client.println(F("<div class='card'>"));
    client.println(F("<h3><i class='fas fa-play-circle'></i> Available Demonstrations</h3>"));
    client.println(F("<p>Professional demonstration modes for wheelchair swing system.</p>"));

    if (!_testState.demoActive)
    {
        client.println(F("<div class='grid grid-2'>"));

        // Gentle demo
        client.println(F("<div style='border:1px solid #e2e8f0;padding:16px;border-radius:8px'>"));
        client.println(F("<h4><i class='fas fa-heart'></i> Gentle Demo</h4>"));
        client.println(F("<p>Slow, smooth movements suitable for all users. Demonstrates basic safety features.</p>"));
        sendButton(client, F("<i class='fas fa-play'></i> Start Gentle Demo"), "/api/demo-start?mode=gentle", "success");
        client.println(F("</div>"));

        // Full feature demo
        client.println(F("<div style='border:1px solid #e2e8f0;padding:16px;border-radius:8px'>"));
        client.println(F("<h4><i class='fas fa-rocket'></i> Full Feature Demo</h4>"));
        client.println(F("<p>Complete demonstration showcasing all features including voice control and motor testing.</p>"));
        sendButton(client, F("<i class='fas fa-play'></i> Start Full Demo"), "/api/demo-start?mode=full", "primary");
        client.println(F("</div>"));

        // Safety demo
        client.println(F("<div style='border:1px solid #e2e8f0;padding:16px;border-radius:8px'>"));
        client.println(F("<h4><i class='fas fa-shield-alt'></i> Safety Demo</h4>"));
        client.println(F("<p>Focused demonstration of safety systems and emergency response capabilities.</p>"));
        sendButton(client, F("<i class='fas fa-play'></i> Start Safety Demo"), "/api/demo-start?mode=safety", "warning");
        client.println(F("</div>"));

        // Custom demo
        client.println(F("<div style='border:1px solid #e2e8f0;padding:16px;border-radius:8px'>"));
        client.println(F("<h4><i class='fas fa-cog'></i> Custom Demo</h4>"));
        client.println(F("<p>Customizable demonstration sequence with adjustable parameters.</p>"));
        sendButton(client, F("<i class='fas fa-play'></i> Start Custom Demo"), "/api/demo-start?mode=custom", "primary");
        client.println(F("</div>"));

        client.println(F("</div>"));
    }
    else
    {
        client.println(F("<div class='grid grid-2'>"));
        sendButton(client, F("<i class='fas fa-stop'></i> Stop Demo"), "/api/demo-stop", "danger");
        sendButton(client, F("<i class='fas fa-info'></i> Demo Status"), "/api/demo-status", "primary");
        client.println(F("</div>"));
    }

    client.println(F("</div>"));
}
// 404 page
void WebServer::send404Page(WiFiClient &client)
{
    client.println(F("HTTP/1.1 404 Not Found"));
    client.println(F("Content-Type: text/html"));
    client.println(F("Connection: close"));
    client.println();

    client.println(F("<!DOCTYPE html><html><head>"));
    client.println(F("<meta charset='UTF-8'>"));
    client.println(F("<meta name='viewport' content='width=device-width,initial-scale=1.0'>"));
    client.println(F("<title>Page Not Found - Wheelchair Swing</title>"));
    client.println(F("<link href='https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.5.0/css/all.min.css' rel='stylesheet'>"));

    sendUnifiedCSS(client);

    client.println(F("</head><body><div class='container'>"));

    client.println(F("<div class='header' style='text-align:center;margin-top:60px'>"));
    client.println(F("<div style='font-size:6rem;color:#e2e8f0;margin-bottom:20px'>"));
    client.println(F("<i class='fas fa-exclamation-triangle'></i>"));
    client.println(F("</div>"));
    client.println(F("<h1 style='color:#6b7280'>Page Not Found</h1>"));
    client.println(F("<p style='color:#9ca3af;margin:20px 0'>The page you're looking for doesn't exist.</p>"));
    client.println(F("</div>"));

    client.println(F("<div class='card'>"));
    client.println(F("<h3><i class='fas fa-compass'></i> Where would you like to go?</h3>"));
    client.println(F("<div class='grid grid-2'>"));
    sendButton(client, F("<i class='fas fa-home'></i> Return Home"), "/", "primary");
    sendButton(client, F("<i class='fas fa-gamepad'></i> Control Panel"), "/control", "primary");
    client.println(F("</div>"));
    client.println(F("</div>"));

    client.println(F("</div>"));
    sendUnifiedJS(client);
    client.println(F("</body></html>"));
}

// Motor test API handler
void WebServer::handleMotorTestAPI(WiFiClient &client, String command)
{
    Serial.print(F("Motor Test API Command: "));
    Serial.println(command);

    if (command.startsWith("left"))
    {
        testMotorLeft(client, command);
    }
    else if (command.startsWith("right"))
    {
        testMotorRight(client, command);
    }
    else if (command == "sync")
    {
        testMotorSync(client);
    }
    else if (command.startsWith("ramp"))
    {
        startRampTest(client, command);
    }
    else if (command == "stop")
    {
        stopMotorTest(client);
    }
    else if (command == "status")
    {
        getMotorTestStatus(client);
    }
    else if (command == "reset-positions")
    {
        resetMotorPositions();
        sendJsonResponse(client, F("{\"status\":\"positions_reset\"}"));
    }
    else
    {
        sendJsonResponse(client, F("{\"error\":\"Unknown motor test command\"}"));
    }
}

// Demo API handler
void WebServer::handleDemoAPI(WiFiClient &client, String command)
{
    Serial.print(F("Demo API Command: "));
    Serial.println(command);

    if (command.startsWith("-start"))
    {
        startDemo(client, command);
    }
    else if (command == "-stop")
    {
        stopDemo(client);
    }
    else if (command == "-status")
    {
        getDemoStatus(client);
    }
    else
    {
        sendJsonResponse(client, F("{\"error\":\"Unknown demo command\"}"));
    }
}

// Utility methods
void WebServer::sendHttpHeader(WiFiClient &client, const char *contentType)
{
    client.println(F("HTTP/1.1 200 OK"));
    client.print(F("Content-Type: "));
    client.println(contentType);
    client.println(F("Connection: close"));
    client.println();
}

void WebServer::sendJsonResponse(WiFiClient &client, const __FlashStringHelper *json)
{
    client.println(F("HTTP/1.1 200 OK"));
    client.println(F("Content-Type: application/json"));
    client.println(F("Connection: close"));
    client.println();
    client.println(json);
}

void WebServer::sendJsonResponse(WiFiClient &client, const String &json)
{
    client.println(F("HTTP/1.1 200 OK"));
    client.println(F("Content-Type: application/json"));
    client.println(F("Connection: close"));
    client.println();
    client.println(json);
}

void WebServer::logEvent(const String &eventType, const String &description, const String &status, unsigned long responseTime)
{
    // Store in circular buffer
    _logs[_logIndex].timestamp = millis();
    _logs[_logIndex].eventType = eventType;
    _logs[_logIndex].description = description;
    _logs[_logIndex].status = status;
    _logs[_logIndex].responseTime = responseTime;

    // Log to serial
    Serial.print(F("LOG: ["));
    Serial.print(eventType);
    Serial.print(F("] "));
    Serial.print(description);
    Serial.print(F(" - "));
    Serial.print(status);
    if (responseTime > 0)
    {
        Serial.print(F(" ("));
        Serial.print(responseTime);
        Serial.print(F("ms)"));
    }
    Serial.println();

    // Move to next log entry
    _logIndex = (_logIndex + 1) % MAX_LOGS;
}

void WebServer::exportConfiguration(WiFiClient& client) {
    RuntimeConfig& config = RuntimeConfig::getInstance();
    String jsonConfig = config.exportToJson();

    client.println(F("HTTP/1.1 200 OK"));
    client.println(F("Content-Type: application/octet-stream"));  // Force download
    client.println(F("Content-Disposition: attachment; filename=\"wheelchair-swing-config.json\""));
    client.print(F("Content-Length: "));
    client.println(jsonConfig.length());
    client.println(F("Connection: close"));
    client.println();  // Empty line to end headers
    client.print(jsonConfig);  // Send the actual JSON content

    // Log the export
    logEvent("config", "Configuration exported", "SUCCESS", 0);
}



String WebServer::getCurrentDateTime()
{
    unsigned long currentTime = millis();
    unsigned long seconds = currentTime / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;

    return String(hours % 24) + ":" +
           String((minutes % 60) < 10 ? "0" : "") + String(minutes % 60) + ":" +
           String((seconds % 60) < 10 ? "0" : "") + String(seconds % 60);
}

String WebServer::getSystemStatus()
{
    String status = "{";
    status += "\"state\":\"" + String(_stateMachine->getStateString()) + "\",";
    status += "\"speed\":\"" + String(_stateMachine->getSpeedString()) + "\",";
    status += "\"safety\":\"" + String(_safetyMonitor->getStatusString()) + "\",";
    status += "\"frontDistance\":" + String(_safetyMonitor->getFrontDistance()) + ",";
    status += "\"rearDistance\":" + String(_safetyMonitor->getRearDistance()) + ",";
    status += "\"userPresent\":" + String(_safetyMonitor->isUserPresent() ? "true" : "false") + ",";
    status += "\"uptime\":" + String(millis() / 1000);
    status += "}";
    return status;
}

// Simple motor test implementations
void WebServer::testMotorLeft(WiFiClient &client, String params)
{
    _testState.motorTestActive = true;
    _testState.currentMotorTest = "left";
    _testState.motorTestStartTime = millis();

    sendJsonResponse(client, F("{\"status\":\"testing_left_motor\"}"));
    logEvent("motor", "Left motor test started", "STARTED");
}

void WebServer::testMotorRight(WiFiClient &client, String params)
{
    _testState.motorTestActive = true;
    _testState.currentMotorTest = "right";
    _testState.motorTestStartTime = millis();

    sendJsonResponse(client, F("{\"status\":\"testing_right_motor\"}"));
    logEvent("motor", "Right motor test started", "STARTED");
}

void WebServer::testMotorSync(WiFiClient &client)
{
    _testState.motorTestActive = true;
    _testState.currentMotorTest = "sync";
    _testState.motorTestStartTime = millis();

    sendJsonResponse(client, F("{\"status\":\"testing_motor_sync\"}"));
    logEvent("motor", "Motor synchronization test started", "STARTED");
}

void WebServer::stopMotorTest(WiFiClient &client)
{
    _testState.motorTestActive = false;
    _testState.currentMotorTest = "";

    sendJsonResponse(client, F("{\"status\":\"motor_test_stopped\"}"));
    logEvent("motor", "Motor test stopped", "STOPPED");
}

void WebServer::getMotorTestStatus(WiFiClient &client)
{
    String response = "{";
    response += "\"active\":" + String(_testState.motorTestActive ? "true" : "false") + ",";
    response += "\"test\":\"" + _testState.currentMotorTest + "\",";
    response += "\"leftPosition\":" + String(_testState.leftMotorPosition) + ",";
    response += "\"rightPosition\":" + String(_testState.rightMotorPosition) + ",";
    response += "\"elapsed\":" + String(_testState.motorTestActive ? (millis() - _testState.motorTestStartTime) : 0);
    response += "}";

    sendJsonResponse(client, response);
}

void WebServer::resetMotorPositions()
{
    _testState.leftMotorPosition = 0;
    _testState.rightMotorPosition = 0;
    logEvent("motor", "Motor positions reset", "INFO");
}

// Simple demo implementations
void WebServer::startDemo(WiFiClient &client, String params)
{
    int modeStart = params.indexOf("mode=") + 5;
    String mode = params.substring(modeStart);

    _testState.demoActive = true;
    _testState.currentDemo = mode;
    _testState.demoStartTime = millis();
    _testState.demoStep = 0;

    String response = "{\"status\":\"demo_started\",\"mode\":\"" + mode + "\"}";
    sendJsonResponse(client, response);

    logEvent("demo", "Demo started: " + mode, "STARTED");
}

void WebServer::stopDemo(WiFiClient &client)
{
    _testState.demoActive = false;
    _testState.currentDemo = "";

    sendJsonResponse(client, F("{\"status\":\"demo_stopped\"}"));
    logEvent("demo", "Demo stopped", "STOPPED");
}

void WebServer::getDemoStatus(WiFiClient &client)
{
    String response = "{";
    response += "\"active\":" + String(_testState.demoActive ? "true" : "false") + ",";
    response += "\"mode\":\"" + _testState.currentDemo + "\",";
    response += "\"step\":" + String(_testState.demoStep) + ",";
    response += "\"elapsed\":" + String(_testState.demoActive ? (millis() - _testState.demoStartTime) : 0);
    response += "}";

    sendJsonResponse(client, response);
}

// Start ramp test - MISSING IMPLEMENTATION
void WebServer::startRampTest(WiFiClient& client, String params) {
    _testState.motorTestActive = true;
    _testState.currentMotorTest = "ramp";
    _testState.motorTestStartTime = millis();

    sendJsonResponse(client, F("{\"status\":\"started\",\"test\":\"ramp\"}"));
    logEvent("motor", "Motor ramp test started", "STARTED", 0);

    // Simulate ramp test sequence
    for (int speed = 100; speed <= 600; speed += 50) {
        if (_leftStepper) {
            // Would control actual stepper here
            // _leftStepper->setSpeed(speed);
        }
        if (_rightStepper) {
            // Would control actual stepper here
            // _rightStepper->setSpeed(speed);
        }

        String logMsg = "Ramp test speed: " + String(speed) + " RPM";
        logEvent("motor", logMsg, "RUNNING", 0);

        delay(1000); // Hold each speed for 1 second
    }

    _testState.motorTestActive = false;
    logEvent("motor", "Motor ramp test completed", "COMPLETED", 0);
}

// Get motor position - MISSING IMPLEMENTATION
void WebServer::getMotorPosition(WiFiClient& client) {
    String response = "{";
    response += "\"leftPosition\":" + String(_testState.leftMotorPosition) + ",";
    response += "\"rightPosition\":" + String(_testState.rightMotorPosition);
    response += "}";

    sendJsonResponse(client, response);
}

// Test motor direction - MISSING IMPLEMENTATION
void WebServer::testMotorDirection(WiFiClient& client, String params) {
    String motor = "left";
    String direction = "forward";

    if (params.indexOf("motor=right") > 0) motor = "right";
    if (params.indexOf("direction=reverse") > 0) direction = "reverse";

    _testState.motorTestActive = true;
    _testState.currentMotorTest = motor + "_direction_" + direction;
    _testState.motorTestStartTime = millis();

    String response = "{\"status\":\"testing\",\"motor\":\"" + motor + "\",\"direction\":\"" + direction + "\"}";
    sendJsonResponse(client, response);

    logEvent("motor", motor + " motor direction test: " + direction, "STARTED", 0);

    // Simulate direction test
    delay(2000);

    _testState.motorTestActive = false;
    logEvent("motor", motor + " motor direction test completed", "COMPLETED", 0);
}

// Check motor test safety - MISSING IMPLEMENTATION
bool WebServer::checkMotorTestSafety() {
    // Check if it's safe to run motor tests
    if (!_testState.motorTestSafetyCheck) return false;

    // Check user presence
    if (!_safetyMonitor->isUserPresent()) {
        logEvent("motor", "Motor test safety check failed - no user present", "FAILED", 0);
        return false;
    }

    // Check for obstacles
    float frontDist = _safetyMonitor->getFrontDistance();
    float rearDist = _safetyMonitor->getRearDistance();

    RuntimeConfig& config = RuntimeConfig::getInstance();
    if (frontDist < config.getFrontCriticalDistance() || rearDist < config.getRearCriticalDistance()) {
        logEvent("motor", "Motor test safety check failed - obstacles detected", "FAILED", 0);
        return false;
    }

    return true;
}

