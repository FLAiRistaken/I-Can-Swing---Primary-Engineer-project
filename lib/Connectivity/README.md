# Connectivity Module

This module manages the network connectivity and web interface for the "I Can Swing" project.

## Components

### WiFiManager

- Handles WiFi connection setup, monitoring, and diagnostics.
- Manages automatic connection retries and timeouts.
- Provides current connection status, signal strength, and network information (e.g. IP address).
- Outputs debugging information to the serial terminal for diagnostics.

### WebServer

- Runs a lightweight HTTP server on port 80.
- Serves user-friendly web pages for system status, remote control, and runtime configuration.
- Supports multiple pages:
    - Home (status/overview)
    - Control Panel (start/stop, speed, doors, alerts)
    - Settings/Configuration (tuning all runtime parameters)
    - Error/404
- Supports REST-style API endpoints for programmatic control and automation (`/api/control`, `/api/config`, `/api/status`).
- Translates web/REST commands into events for the StateMachine and logs user actions.
- Displays current sensor data and recent activity logs.

## Usage

- **During setup:**
    - Call `WiFiManager::begin(ssid, password)` to establish WiFi connection.
    - Call `WebServer::begin()` to start the HTTP server.
- **In the main loop:**
    - Call `WiFiManager::isConnected()` to monitor WiFi health.
    - Call `WebServer::handleClient()` regularly to process incoming HTTP requests.

## Features

- Robust connection management and status reporting.
- User-facing web dashboard with live monitoring and remote control.
- Dynamic access to runtime configuration (safety, motion, audio settings) via the web.
- RESTful API interface for integration with other systems or accessibility tools.
- Recent event/activity log for monitoring and troubleshooting.

## Future Enhancements

- Add secure authentication and HTTPS support for web access.
- Expand REST API to provide richer diagnostics and fine-grained control.
- Add real-time data visualisation (swing motion, sensor graphs) to the web UI.

