#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <LittleFS.h>

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
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
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
            background-color: #ffffff;
            border: 4px solid #1e1e1e;
            border-radius: 15px;
            box-shadow: 8px 8px 0px #5c3a7e;
            filter: url('#wriggly-container');
            transform: rotate(0.5deg);
            z-index: -1;
        }

        h1 {
            font-family: 'Gluten', Arial, sans-serif;
            color: #1e1e1e;
            text-align: center;
            margin-bottom: 30px;
            font-size: 2.5rem;
            font-weight: 700;
            text-shadow: 2px 2px 0px #667eea;
            font-variation-settings: 'wght' 700, 'slnt' -10;
        }

        /* --- Torn Paper Network Table --- */
        .network-table {
            position: relative;
            width: 100%;
            margin-bottom: 20px;
            border-collapse: separate;
            border-spacing: 0;
            z-index: 1;
        }

        .network-table::before {
            content: "";
            position: absolute;
            top: 0; left: 0; right: 0; bottom: 0;
            background-color: #f8f9fa;
            border: 3px solid #1e1e1e;
            border-radius: 12px;
            box-shadow: 4px 4px 0px #1e1e1e;
            filter: url('#wriggly-table');
            transform: rotate(-0.3deg);
            z-index: -1;
        }

        .network-table tr {
            position: relative;
            cursor: pointer;
            transition: background 0.2s;
            z-index: 1;
        }

        .network-table tr:hover {
            background: #ffffff;
        }

        .network-table tr.selected {
            background: #e7f0ff;
        }

        .network-table td {
            padding: 12px;
            border-bottom: 1px solid #e0e0e0;
        }

        .network-table tr:last-child td {
            border-bottom: none;
        }

        .network-table td:first-child {
            width: 40px;
            text-align: center;
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
            accent-color: #667eea;
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

        /* --- Torn Paper Password Input --- */
        input[type="password"] {
            position: relative;
            width: 100%;
            padding: 14px;
            border: 3px solid #1e1e1e;
            border-radius: 10px;
            font-family: 'Fredoka', Arial, sans-serif;
            font-size: 16px;
            background-color: #ffffff;
            transition: all 0.2s;
            box-shadow: 3px 3px 0px #1e1e1e;
            z-index: 1;
        }

        input[type="password"]:focus {
            outline: none;
            border-color: #667eea;
            box-shadow: 5px 5px 0px #667eea;
            transform: translateY(-1px);
        }

        /* --- Torn Paper Status Message --- */
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
            background-color: #e8f5e9;
            border: 3px solid #4caf50;
            border-radius: 10px;
            box-shadow: 3px 3px 0px #2e7d32;
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

        <table class="network-table" id="network-table">
            <tbody id="network-list">
                <tr>
                    <td colspan="3" class="loading">Scanning networks...</td>
                </tr>
            </tbody>
        </table>

        <div class="controls">
            <button class="refresh" onclick="refreshNetworks()">🔄 Refresh Networks</button>
            <input type="password" id="wifi-password" placeholder="Network Password">
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

            if (!selectedNetwork) {
                alert('Please select a network first');
                return;
            }

            // TODO: Send credentials to device
            alert('Connect functionality coming next!\\nNetwork: ' + selectedNetwork);
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
        </defs>
    </svg>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", index_html);
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
  Serial.println("Blossom - WiFi Setup Portal");
  Serial.println("=================================\n");

  // Initialize LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Initialize LittleFS for font file
  if (!LittleFS.begin()) {
    Serial.println("⚠ LittleFS mount failed - font won't load");
    Serial.println("  Run: pio run --target uploadfs");
  } else {
    Serial.println("✓ LittleFS mounted");
  }

  // Start Access Point
  Serial.println("Starting Access Point...");
  Serial.print("SSID: ");
  Serial.println(AP_SSID);

  // Set WiFi mode
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

  // Configure web server routes
  server.on("/", handleRoot);
  server.on("/fonts/Gluten.ttf", handleFont);
  server.on("/fonts/Fredoka.ttf", handleFredokaFont);
  server.on("/api/scan", handleScan);  // WiFi scan endpoint
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
  // Process DNS requests (for captive portal)
  dnsServer.processNextRequest();

  // Handle web server requests
  server.handleClient();

  // Optional: Blink LED to show we're alive
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 2000) {
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    lastBlink = millis();
  }
}
