#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include <LEAmDNS.h>

// Access Point configuration
const char* AP_SSID = "Blossom_Setup";
// No password - open network for provisioning

// Web server on port 80
WebServer server(80);

// DNS server for captive portal (port 53)
DNSServer dnsServer;
const byte DNS_PORT = 53;

// LED pin (Pico W uses WiFi chip LED)
#ifndef LED_BUILTIN
#define LED_BUILTIN 25
#endif

// Credential storage
const char* CRED_FILE = "/wifi_creds.txt";

// Operation modes
enum SystemMode {
  MODE_PROVISIONING,
  MODE_CONNECTED
};

SystemMode currentMode = MODE_PROVISIONING;

// Deferred reboot - lets HTTP response flush before AP goes down
bool rebootPending = false;
unsigned long rebootAt = 0;

// Factory reset button tracking
unsigned long bootselPressStart = 0;
bool bootselPressedLastLoop = false;
const unsigned long FACTORY_RESET_HOLD_TIME = 5000;  // 5 seconds

// HTML page with beautiful redesign
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Blossom Setup</title>
    <style>
        @font-face {
            font-family: 'Gluten';
            src: url('/fonts/Gluten.ttf') format('truetype');
            font-weight: 100 900;
            font-style: normal;
        }

        @font-face {
            font-family: 'Fredoka';
            src: url('/fonts/Fredoka.ttf') format('truetype');
            font-weight: 300 700;
            font-style: normal;
        }

        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: 'Fredoka', Arial, sans-serif;
            background: linear-gradient(135deg, #3d53b8 0%, #4e306b 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
        }

        /* --- Torn Paper Container --- */
        .container {
            position: relative;
            padding: 30px;
            max-width: 500px;
            width: 100%;
            z-index: 1;
        }

        .container::before {
            content: "";
            position: absolute;
            top: 0; left: 0; right: 0; bottom: 0;
            background-color: #EAD8C1;
            border: 3px solid #83786bc2;
            border-radius: 15px;
            box-shadow: 8px 8px 0px #3b2b188a;
            filter: url('#wriggly-container');
            transform: rotate(0.5deg);
            z-index: -1;
        }

        h1 {
            font-family: 'Gluten', Arial, sans-serif;
            color: #1e1e1e;
            text-align: center;
            margin-bottom: 10px;
            font-size: 2.5rem;
            font-weight: 700;
            text-shadow: 2px 2px 0px #667eea;
            font-variation-settings: 'wght' 700, 'slnt' -10;
        }

        /* --- Torn Paper Network Table Wrapper --- */
        .network-table-wrapper {
            position: relative;
            margin-bottom: 20px;
            padding: 8px; /* Inset content from border */
            z-index: 1;
        }

        .network-table-wrapper::before {
            content: "";
            position: absolute;
            top: 0; left: 0; right: 0; bottom: 0;
            background-color: #e9dfd3;
            border: 3px solid #83786bc2;
            border-radius: 12px;
            box-shadow: -2px -2px 0px #3b2b188a;
            filter: url('#wriggly-table');
            transform: rotate(-0.3deg);
            z-index: -1;
            pointer-events: none;
        }

        .network-table {
            width: 100%;
            border-collapse: separate;
            border-spacing: 0;
            background: transparent;
        }

        .network-table tr {
            cursor: pointer;
            transition: background 0.2s;
        }

        .network-table tr:hover {
            background: #e3e4c0;
        }

        .network-table tr.selected {
            background: #ece7e0;
        }

        .network-table td {
            padding: 12px;
        }

        .network-table tr:last-child td {
            border-bottom: none;
        }

        .network-table tr:first-child td:first-child {
            border-radius: 8px 0 0 0; /* Round top-left on first row */
        }

        .network-table tr:first-child td:last-child {
            border-radius: 0 8px 0 0; /* Round top-right on first row */
        }

        .network-table tr:last-child td:first-child {
            border-radius: 0 0 0 8px; /* Round bottom-left on last row */
        }

        .network-table tr:last-child td:last-child {
            border-radius: 0 0 8px 0; /* Round bottom-right on last row */
        }

        .network-table td:first-child {
            width: 40px;
            text-align: center;
            filter: url('#wriggly-radio'); /* Hand-drawn radio buttons */
        }

        .network-table td:nth-child(2) {
            font-family: 'Fredoka', Arial, sans-serif;
            font-size: 16px;
            color: #1e1e1e;
            font-weight: 500;
        }

        .network-table td:last-child {
            width: 50px;
            text-align: center;
        }

        input[type="radio"] {
            width: 20px;
            height: 20px;
            cursor: pointer;
            accent-color: #1e1e1e;
        }

        .wifi-icon {
            display: inline-block;
            width: 24px;
            height: 24px;
        }

        .wifi-icon svg {
            width: 100%;
            height: 100%;
        }

        .wifi-icon .bar {
            fill: none;
            stroke: #cbd5e1;
            stroke-width: 2;
            stroke-linecap: round;
        }

        .wifi-icon .dot {
            fill: #cbd5e1;
        }

        /* Signal strength colors */
        .strength-excellent .dot { fill: #10b981; }
        .strength-excellent .bar-1,
        .strength-excellent .bar-2,
        .strength-excellent .bar-3 { stroke: #10b981; }

        .strength-good .dot { fill: #34d399; }
        .strength-good .bar-1,
        .strength-good .bar-2 { stroke: #34d399; }

        .strength-fair .dot { fill: #f59e0b; }
        .strength-fair .bar-1 { stroke: #f59e0b; }

        .strength-weak .dot { fill: #ef4444; }

        .controls {
            display: flex;
            flex-direction: column;
            gap: 16px;
            margin-top: 20px;
        }

        /* --- Torn Paper Button Base --- */
        button {
            position: relative;
            font-family: 'Fredoka', Arial, sans-serif;
            font-weight: 600;
            font-size: 16px;
            color: #1e1e1e;
            background: transparent;
            border: 0;
            padding: 14px 24px;
            cursor: pointer;
            transition: transform 0.15s ease;
            z-index: 1;
        }

        button::before {
            content: "";
            position: absolute;
            top: 0; left: 0; right: 0; bottom: 0;
            background-color: #6bf1a4; /* Default mint color */
            border: 3px solid #1e1e1e;
            border-radius: 10px;
            box-shadow: 4px 4px 0px #1e1e1e;
            z-index: -1;
            transition: background-color 0.2s ease;
        }

        button.refresh::before {
            filter: url('#wriggly-btn-1');
            transform: rotate(0.8deg);
            background-color: #cca4ff; /* Lavender */
        }

        button.connect {
            padding: 16px 24px;
            font-size: 18px;
            font-weight: 700;
        }

        button.connect::before {
            filter: url('#wriggly-btn-2');
            transform: rotate(-1deg);
            background-color: #ff858d; /* Coral */
        }

        button:hover {
            transform: scale(1.05) translateY(-2px);
        }

        button:hover::before {
            background-color: #ffffff;
        }

        button:active {
            transform: translate(2px, 2px);
        }

        button:active::before {
            box-shadow: 2px 2px 0px #1e1e1e;
        }

        /* --- Torn Paper Password Input Wrapper --- */
        .input-wrapper {
            position: relative;
            width: 100%;
        }

        .input-wrapper::before {
            content: "";
            position: absolute;
            top: 0; left: 0; right: 0; bottom: 0;
            background-color: #ece7e0;
            border: 3px solid #83786bc2;
            border-radius: 10px;
            box-shadow: -2px -2px 0px #3b2b188a;
            filter: url('#wriggly-input');
            transform: rotate(-0.2deg);
            z-index: -1;
            pointer-events: none;
            transition: all 0.2s;
        }

        input[type="password"] {
            width: 100%;
            padding: 14px;
            border: 0;
            background: transparent;
            font-family: 'Fredoka', Arial, sans-serif;
            font-size: 16px;
            color: #1e1e1e;
            transition: all 0.2s;
        }

        input[type="password"]:focus {
            outline: none;
        }

        .input-wrapper:focus-within::before {
            box-shadow: -3px -3px 0px #3b2b188a;
            transform: rotate(-0.2deg) translateY(-1px);
        }

        /* --- Status Message Stamp --- */
        .status {
            position: relative;
            text-align: center;
            padding: 12px;
            color: #2e7d32;
            margin-bottom: 20px;
            font-family: 'Fredoka', Arial, sans-serif;
            font-weight: 600;
            display: none;
            z-index: 1;
        }

        .status::before {
            content: "";
            position: absolute;
            top: 0; left: 0; right: 0; bottom: 0;
            background-color: #ead8c100; /* Transparent */
            border: 3px solid #2e7d32;
            border-radius: 10px;
            filter: url('#wriggly-status');
            transform: rotate(-0.5deg);
            z-index: -1;
        }

        .loading {
            text-align: center;
            padding: 40px;
            color: #666;
            font-family: 'Fredoka', Arial, sans-serif;
            font-weight: 400;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🪷 Blossom 🪷</h1>

        <div class="status" id="status-message"></div>

        <div class="network-table-wrapper">
            <table class="network-table" id="network-table">
                <tbody id="network-list">
                    <tr>
                        <td colspan="3" class="loading">Scanning networks...</td>
                    </tr>
                </tbody>
            </table>
        </div>

        <div class="controls">
            <button class="refresh" onclick="refreshNetworks()">Refresh Networks</button>
            <div class="input-wrapper">
                <input type="password" id="wifi-password" placeholder="Network Password">
            </div>
            <button class="connect" onclick="connectWiFi()">Connect to Network</button>
        </div>
    </div>

    <script>
        let selectedNetwork = null;

        // Get WiFi icon SVG based on signal strength
        function getWiFiIcon(rssi) {
            let strengthClass = 'strength-weak';
            if (rssi >= -50) strengthClass = 'strength-excellent';
            else if (rssi >= -60) strengthClass = 'strength-good';
            else if (rssi >= -70) strengthClass = 'strength-fair';

            return `
                <div class="wifi-icon ${strengthClass}">
                    <svg viewBox="0 0 24 24" xmlns="http://www.w3.org/2000/svg">
                        <circle class="dot" cx="12" cy="20" r="1.5" />
                        <path class="bar bar-1" d="M9 16 A 4.5 4.5 0 0 1 15 16" />
                        <path class="bar bar-2" d="M6 12 A 9 9 0 0 1 18 12" />
                        <path class="bar bar-3" d="M3 8 A 13.5 13.5 0 0 1 21 8" />
                    </svg>
                </div>
            `;
        }

        // Scan for WiFi networks on page load
        window.onload = function() {
            scanNetworks();
        };

        function refreshNetworks() {
            selectedNetwork = null;
            document.getElementById('network-list').innerHTML =
                '<tr><td colspan="3" class="loading">Scanning networks...</td></tr>';
            scanNetworks();
        }

        function scanNetworks() {
            fetch('/api/scan')
                .then(response => response.json())
                .then(data => {
                    const tbody = document.getElementById('network-list');
                    tbody.innerHTML = '';

                    if (data.networks && data.networks.length > 0) {
                        data.networks.forEach((network, index) => {
                            const row = document.createElement('tr');
                            row.onclick = () => selectNetwork(network.ssid, row);

                            row.innerHTML = `
                                <td><input type="radio" name="network" value="${network.ssid}" id="net-${index}"></td>
                                <td><label for="net-${index}" style="cursor: pointer;">${network.ssid}</label></td>
                                <td>${getWiFiIcon(network.rssi)}</td>
                            `;

                            tbody.appendChild(row);
                        });

                        document.getElementById('status-message').style.display = 'block';
                        document.getElementById('status-message').textContent =
                            `✓ Found ${data.networks.length} network(s)`;
                    } else {
                        tbody.innerHTML = '<tr><td colspan="3" class="loading">No networks found</td></tr>';
                    }
                })
                .catch(error => {
                    console.error('Scan error:', error);
                    document.getElementById('network-list').innerHTML =
                        '<tr><td colspan="3" class="loading">Scan failed - click refresh</td></tr>';
                });
        }

        function selectNetwork(ssid, row) {
            selectedNetwork = ssid;

            // Remove selected class from all rows
            document.querySelectorAll('.network-table tr').forEach(r => {
                r.classList.remove('selected');
            });

            // Add selected class to clicked row
            row.classList.add('selected');

            // Check the radio button
            row.querySelector('input[type="radio"]').checked = true;
        }

        function connectWiFi() {
            const password = document.getElementById('wifi-password').value;
            const statusEl = document.getElementById('status-message');

            if (!selectedNetwork) {
                statusEl.style.display = 'block';
                statusEl.style.borderColor = '#d32f2f';
                statusEl.style.color = '#d32f2f';
                statusEl.textContent = '✗ Please select a network first';
                return;
            }

            statusEl.style.display = 'block';
            statusEl.style.borderColor = '#1976d2';
            statusEl.style.color = '#1976d2';
            statusEl.textContent = '⟳ Saving credentials...';

            fetch('/api/connect', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ ssid: selectedNetwork, password: password })
            })
            .then(r => r.json())
            .then(data => {
                statusEl.style.borderColor = '#2e7d32';
                statusEl.style.color = '#2e7d32';
                statusEl.textContent = '✓ Saved! Device is rebooting to connect...';
            })
            .catch(() => {
                statusEl.style.borderColor = '#d32f2f';
                statusEl.style.color = '#d32f2f';
                statusEl.textContent = '✗ Could not reach device - please try again';
            });
        }
    </script>

    <!-- SVG Filters for Torn Paper Effect -->
    <svg style="position: absolute; width: 0; height: 0;">
        <defs>
            <filter id="wriggly-container">
                <feTurbulence type="fractalNoise" baseFrequency="0.04" numOctaves="3" seed="99" result="noise" />
                <feDisplacementMap in="SourceGraphic" in2="noise" scale="10" xChannelSelector="R" yChannelSelector="G" />
            </filter>
            
            <filter id="wriggly-table">
                <feTurbulence type="fractalNoise" baseFrequency="0.05" numOctaves="3" seed="42" result="noise" />
                <feDisplacementMap in="SourceGraphic" in2="noise" scale="8" xChannelSelector="R" yChannelSelector="G" />
            </filter>
            
            <filter id="wriggly-btn-1">
                <feTurbulence type="fractalNoise" baseFrequency="0.06" numOctaves="3" seed="12" result="noise" />
                <feDisplacementMap in="SourceGraphic" in2="noise" scale="7" xChannelSelector="R" yChannelSelector="G" />
            </filter>
            
            <filter id="wriggly-btn-2">
                <feTurbulence type="fractalNoise" baseFrequency="0.06" numOctaves="3" seed="78" result="noise" />
                <feDisplacementMap in="SourceGraphic" in2="noise" scale="7" xChannelSelector="R" yChannelSelector="G" />
            </filter>
            
            <filter id="wriggly-status">
                <feTurbulence type="fractalNoise" baseFrequency="0.05" numOctaves="3" seed="33" result="noise" />
                <feDisplacementMap in="SourceGraphic" in2="noise" scale="6" xChannelSelector="R" yChannelSelector="G" />
            </filter>
            
            <filter id="wriggly-radio">
                <feTurbulence type="fractalNoise" baseFrequency="0.15" numOctaves="2" seed="87" result="noise" />
                <feDisplacementMap in="SourceGraphic" in2="noise" scale="4" xChannelSelector="R" yChannelSelector="G" />
            </filter>
            
            <filter id="wriggly-input">
                <feTurbulence type="fractalNoise" baseFrequency="0.05" numOctaves="3" seed="55" result="noise" />
                <feDisplacementMap in="SourceGraphic" in2="noise" scale="8" xChannelSelector="R" yChannelSelector="G" />
            </filter>
        </defs>
    </svg>
</body>
</html>
)rawliteral";

// Connected mode HTML - simple Hello World page
const char connected_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Blossom</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            margin: 0;
            padding: 20px;
        }
        .container {
            background: white;
            padding: 40px;
            border-radius: 20px;
            box-shadow: 0 10px 30px rgba(0,0,0,0.3);
            text-align: center;
            max-width: 500px;
        }
        h1 {
            color: #764ba2;
            margin: 0 0 20px 0;
            font-size: 2.5rem;
        }
        p {
            color: #555;
            font-size: 1.1rem;
            margin: 10px 0;
        }
        .status {
            background: #e8f5e9;
            border-left: 4px solid #4caf50;
            padding: 15px;
            margin: 20px 0;
            text-align: left;
        }
        .status strong {
            color: #2e7d32;
        }
        code {
            background: #f5f5f5;
            padding: 2px 6px;
            border-radius: 3px;
            font-family: monospace;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🪷 Blossom 🪷</h1>
        <p>Your ambient light display is connected!</p>
        <div class="status">
            <strong>✓ Status:</strong> Online<br>
            <strong>📡 Network:</strong> <span id="ssid">Loading...</span><br>
            <strong>🌐 IP:</strong> <span id="ip">Loading...</span><br>
            <strong>🔗 Hostname:</strong> <code>blossom.local</code>
        </div>
        <p style="color: #888; font-size: 0.9rem;">API endpoint: <code>http://blossom.local/api/effect</code></p>
    </div>
    <script>
        fetch('/api/status')
            .then(r => r.json())
            .then(data => {
                document.getElementById('ssid').textContent = data.ssid || 'Unknown';
                document.getElementById('ip').textContent = data.ip || 'Unknown';
            })
            .catch(e => console.error('Status fetch failed:', e));
    </script>
</body>
</html>
)rawliteral";

// Helper: Save WiFi credentials to LittleFS
bool saveCredentials(const String& ssid, const String& password) {
  File file = LittleFS.open(CRED_FILE, "w");
  if (!file) {
    Serial.println("✗ Failed to open credentials file for writing");
    return false;
  }
  
  file.println(ssid);
  file.println(password);
  file.close();
  
  Serial.println("✓ Credentials saved to flash");
  return true;
}

// Helper: Load WiFi credentials from LittleFS
bool loadCredentials(String& ssid, String& password) {
  if (!LittleFS.exists(CRED_FILE)) {
    Serial.println("No credentials file found");
    return false;
  }
  
  File file = LittleFS.open(CRED_FILE, "r");
  if (!file) {
    Serial.println("✗ Failed to open credentials file for reading");
    return false;
  }
  
  ssid = file.readStringUntil('\n');
  password = file.readStringUntil('\n');
  file.close();
  
  // Trim whitespace/newlines
  ssid.trim();
  password.trim();
  
  if (ssid.length() == 0) {
    Serial.println("✗ Invalid credentials in file");
    return false;
  }
  
  Serial.println("✓ Credentials loaded from flash");
  return true;
}

// Helper: Clear stored credentials
void clearCredentials() {
  if (LittleFS.exists(CRED_FILE)) {
    LittleFS.remove(CRED_FILE);
    Serial.println("✓ Credentials cleared");
  }
}

// Helper: Attempt to connect to WiFi (blocking - used at boot time only)
bool connectToWiFi(const String& ssid, const String& password, int timeoutSeconds = 15) {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  for (int i = 0; i < password.length(); i++) {
    Serial.print("*");
  }
  Serial.println();
  
  WiFi.mode(WIFI_STA);  // Boot-time: pure STA, no AP needed
  WiFi.begin(ssid.c_str(), password.c_str());
  
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startTime > timeoutSeconds * 1000) {
      Serial.println("\n✗ Connection timeout");
      return false;
    }
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\n✓ WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  return true;
}

void handleRoot() {
  if (currentMode == MODE_PROVISIONING) {
    server.send(200, "text/html", index_html);
  } else {
    server.send(200, "text/html", connected_html);
  }
}

void handleFont() {
  // Serve Gluten font file from LittleFS
  File file = LittleFS.open("/fonts/Gluten.ttf", "r");
  if (file) {
    server.streamFile(file, "font/ttf");
    file.close();
  } else {
    server.send(404, "text/plain", "Font not found");
  }
}

void handleFredokaFont() {
  // Serve Fredoka font file from LittleFS
  File file = LittleFS.open("/fonts/Fredoka.ttf", "r");
  if (file) {
    server.streamFile(file, "font/ttf");
    file.close();
  } else {
    server.send(404, "text/plain", "Font not found");
  }
}

void handleConnect() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"No data received\"}");
    return;
  }
  
  String body = server.arg("plain");
  
  // Simple JSON parsing
  int ssidStart = body.indexOf("\"ssid\":\"") + 8;
  int ssidEnd = body.indexOf("\"", ssidStart);
  int passStart = body.indexOf("\"password\":\"") + 12;
  int passEnd = body.indexOf("\"", passStart);
  
  if (ssidStart < 8 || ssidEnd < 0 || passStart < 12 || passEnd < 0) {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(400, "application/json", "{\"error\":\"Invalid JSON format\"}");
    return;
  }
  
  String ssid = body.substring(ssidStart, ssidEnd);
  String password = body.substring(passStart, passEnd);
  
  Serial.print("Connect request - SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  for (int i = 0; i < password.length(); i++) Serial.print("*");
  Serial.println();
  
  // Save credentials now while AP is fully up (no radio contention)
  saveCredentials(ssid, password);
  
  // Respond immediately - AP is still alive so response is guaranteed to arrive
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", "{\"status\":\"saved\"}");
  
  // Defer the reboot so loop() can finish flushing the TCP response first
  rebootPending = true;
  rebootAt = millis() + 1500;
  Serial.println("Credentials saved. Rebooting in 1.5s to test connection...");
}

void handleStatus() {
  String json = "{";
  json += "\"status\":\"connected\",";
  json += "\"ssid\":\"";
  json += WiFi.SSID();
  json += "\",";
  json += "\"ip\":\"";
  json += WiFi.localIP().toString();
  json += "\",";
  json += "\"rssi\":";
  json += String(WiFi.RSSI());
  json += "}";
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

void handleScan() {
  Serial.println("WiFi scan requested...");

  // Perform WiFi scan
  int numNetworks = WiFi.scanNetworks();

  Serial.print("Scan complete. Found ");
  Serial.print(numNetworks);
  Serial.println(" networks.");

  // Build JSON response with filtering and deduplication
  String json = "{\"networks\":[";
  bool firstNetwork = true;

  // Track SSIDs we've already added (for deduplication)
  String addedSSIDs[50];  // Max 50 unique networks
  int addedCount = 0;

  for (int i = 0; i < numNetworks && i < 50; i++) {
    String ssid = WiFi.SSID(i);
    int rssi = WiFi.RSSI(i);

    // Filter out empty/hidden SSIDs
    if (ssid.length() == 0) {
      Serial.println("Skipping hidden network");
      continue;
    }

    // Check for duplicate SSID (keep strongest signal)
    bool isDuplicate = false;
    for (int j = 0; j < addedCount; j++) {
      if (addedSSIDs[j] == ssid) {
        isDuplicate = true;
        Serial.print("Duplicate SSID found: ");
        Serial.println(ssid);
        break;
      }
    }

    if (isDuplicate) {
      continue;  // Skip this duplicate
    }

    // Add to JSON
    if (!firstNetwork) json += ",";
    json += "{";
    json += "\"ssid\":\"";
    json += ssid;
    json += "\",\"rssi\":";
    json += String(rssi);
    json += ",\"encryption\":";
    json += String(WiFi.encryptionType(i));
    json += "}";

    // Track this SSID
    addedSSIDs[addedCount++] = ssid;
    firstNetwork = false;
  }

  json += "]}";

  Serial.print("Filtered to ");
  Serial.print(addedCount);
  Serial.println(" unique networks");

  // Send JSON response with CORS headers
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);

  // Clean up
  WiFi.scanDelete();
}

void handleNotFound() {
  // Redirect all unknown requests to root (helps with captive portal detection)
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

void setup() {
  // Initialize serial for debugging
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n=================================");
  Serial.println("Blossom - Programmable Light Display");
  Serial.println("=================================\n");

  // Initialize LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Initialize LittleFS
  if (!LittleFS.begin()) {
    Serial.println("⚠ LittleFS mount failed");
    Serial.println("  Run: pio run --target uploadfs");
  } else {
    Serial.println("✓ LittleFS mounted");
  }

  // Check for stored credentials
  String savedSSID, savedPassword;
  bool hasCredentials = loadCredentials(savedSSID, savedPassword);

  if (hasCredentials) {
    // Try to connect to saved network
    Serial.println("Found saved credentials, attempting connection...");
    
    if (connectToWiFi(savedSSID, savedPassword, 20)) {
      // Successfully connected!
      currentMode = MODE_CONNECTED;
      digitalWrite(LED_BUILTIN, HIGH);
      
      // Initialize mDNS
      if (MDNS.begin("blossom")) {
        Serial.println("✓ mDNS responder started");
        Serial.println("  Hostname: blossom.local");
        MDNS.addService("http", "tcp", 80);
      } else {
        Serial.println("✗ mDNS setup failed");
      }
      
      // Configure web server for connected mode
      server.on("/", handleRoot);
      server.on("/api/status", handleStatus);
      server.onNotFound(handleNotFound);
      
      server.begin();
      Serial.println("✓ Web server started (connected mode)");
      Serial.println("\n=================================");
      Serial.println("✓ CONNECTED TO WIFI");
      Serial.print("Network: ");
      Serial.println(WiFi.SSID());
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
      Serial.println("Hostname: blossom.local");
      Serial.println("=================================\n");
      
      return;  // Skip provisioning setup
    } else {
      // Connection failed, clear bad credentials and fall through to provisioning
      Serial.println("✗ Saved credentials failed, clearing and starting provisioning");
      clearCredentials();
    }
  } else {
    Serial.println("No saved credentials found");
  }

  // Start provisioning mode
  currentMode = MODE_PROVISIONING;
  Serial.println("Starting Provisioning Mode...");
  Serial.print("SSID: ");
  Serial.println(AP_SSID);

  // Pure AP mode - no radio contention with STA during provisioning
  WiFi.mode(WIFI_AP);

  // Configure AP IP address BEFORE starting AP
  IPAddress local_IP(192, 168, 4, 1);
  IPAddress gateway(192, 168, 4, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.softAPConfig(local_IP, gateway, subnet);

  // Start Access Point with configured IP
  // Call softAP with only SSID (no password) for open network
  bool apStarted = WiFi.softAP(AP_SSID);

  if (apStarted) {
    Serial.println("✓ Access Point started successfully!");
    digitalWrite(LED_BUILTIN, HIGH);  // Turn on LED when AP is active
  } else {
    Serial.println("✗ Failed to start Access Point");
  }

  // Display network info
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  Serial.println();

  // Start DNS server for captive portal
  // This makes the page auto-open when connecting to WiFi
  dnsServer.start(DNS_PORT, "*", IP);
  Serial.println("✓ DNS server started (captive portal active)");

  // Configure web server routes for provisioning mode
  server.on("/", handleRoot);
  server.on("/fonts/Gluten.ttf", handleFont);
  server.on("/fonts/Fredoka.ttf", handleFredokaFont);
  server.on("/api/scan", handleScan);
  server.on("/api/connect", HTTP_POST, handleConnect);
  server.onNotFound(handleNotFound);

  // Start web server
  server.begin();
  Serial.println("✓ Web server started on port 80");
  Serial.println("\n=================================");
  Serial.println("Connect to WiFi: Blossom_Setup");
  Serial.println("Open browser: http://192.168.4.1");
  Serial.println("=================================\n");
}

void loop() {
  // Factory reset: Check BOOTSEL button
  bool bootselPressed = BOOTSEL;
  
  if (bootselPressed && !bootselPressedLastLoop) {
    // Button just pressed - start timer
    bootselPressStart = millis();
    Serial.println("\nBOOTSEL button pressed - hold for 5 seconds to factory reset...");
  }
  
  if (bootselPressed && bootselPressedLastLoop) {
    // Button is being held - check duration
    unsigned long holdDuration = millis() - bootselPressStart;
    
    // Rapid LED blink during hold to give feedback
    digitalWrite(LED_BUILTIN, (millis() / 100) % 2);
    
    // Print countdown every second
    static unsigned long lastCountdown = 0;
    if (millis() - lastCountdown > 1000) {
      int secondsLeft = (FACTORY_RESET_HOLD_TIME - holdDuration) / 1000;
      if (secondsLeft >= 0) {
        Serial.print("Factory reset in ");
        Serial.print(secondsLeft + 1);
        Serial.println(" seconds...");
      }
      lastCountdown = millis();
    }
    
    if (holdDuration >= FACTORY_RESET_HOLD_TIME) {
      // Factory reset triggered!
      Serial.println("\n=================================");
      Serial.println("FACTORY RESET TRIGGERED");
      Serial.println("=================================");
      
      // Flash LED rapidly to confirm
      for (int i = 0; i < 10; i++) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(50);
        digitalWrite(LED_BUILTIN, LOW);
        delay(50);
      }
      
      // Clear credentials
      clearCredentials();
      
      Serial.println("Rebooting to provisioning mode...");
      delay(500);
      rp2040.reboot();
    }
  }
  
  if (!bootselPressed && bootselPressedLastLoop) {
    // Button released before 5 seconds
    unsigned long holdDuration = millis() - bootselPressStart;
    if (holdDuration < FACTORY_RESET_HOLD_TIME) {
      Serial.println("BOOTSEL button released - factory reset cancelled");
    }
  }
  
  bootselPressedLastLoop = bootselPressed;

  // Deferred reboot - fires after HTTP response has had time to flush
  if (rebootPending && millis() >= rebootAt) {
    rp2040.reboot();
  }

  // Normal operation based on mode
  if (currentMode == MODE_PROVISIONING) {
    // Process DNS requests (for captive portal)
    dnsServer.processNextRequest();
    
    // Blink LED in provisioning mode (unless BOOTSEL is pressed)
    if (!bootselPressed) {
      static unsigned long lastBlink = 0;
      if (millis() - lastBlink > 2000) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        lastBlink = millis();
      }
    }
  } else {
    // Connected mode - update mDNS
    MDNS.update();
    
    // Keep LED solid on when connected (unless BOOTSEL is pressed)
    if (!bootselPressed) {
      digitalWrite(LED_BUILTIN, HIGH);
    }
  }

  // Handle web server requests (both modes)
  server.handleClient();
}
