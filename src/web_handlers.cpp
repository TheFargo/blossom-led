#include "web_handlers.h"
#include "credentials.h"
#include "led_controller.h"
#include <LittleFS.h>
#include <WiFi.h>

// Stream a file from LittleFS to the client
static void serveFile(const char* path, const char* contentType) {
  File file = LittleFS.open(path, "r");
  if (file) {
    server.streamFile(file, contentType);
    file.close();
  } else {
    server.send(404, "text/plain", "File not found");
  }
}

static void handleProvisioningRoot() {
  serveFile("/provisioning.html", "text/html");
}

static void handleConnectedRoot() {
  serveFile("/connected.html", "text/html");
}

static void handleGlutenFont() {
  serveFile("/fonts/Gluten.ttf", "font/ttf");
}

static void handleFredokaFont() {
  serveFile("/fonts/Fredoka.ttf", "font/ttf");
}

static void handleConnect() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"No data received\"}");
    return;
  }

  String body = server.arg("plain");

  int ssidStart = body.indexOf("\"ssid\":\"") + 8;
  int ssidEnd   = body.indexOf("\"", ssidStart);
  int passStart = body.indexOf("\"password\":\"") + 12;
  int passEnd   = body.indexOf("\"", passStart);

  if (ssidStart < 8 || ssidEnd < 0 || passStart < 12 || passEnd < 0) {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(400, "application/json", "{\"error\":\"Invalid JSON format\"}");
    return;
  }

  String ssid     = body.substring(ssidStart, ssidEnd);
  String password = body.substring(passStart, passEnd);

  Serial.print("Connect request - SSID: ");
  Serial.println(ssid);
  Serial.print("Password: ");
  for (int i = 0; i < password.length(); i++) Serial.print("*");
  Serial.println();

  // Save credentials while AP is fully up — no radio contention
  saveCredentials(ssid, password);

  // Respond before the AP disappears
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", "{\"status\":\"saved\"}");

  // Deferred reboot via main.cpp — gives TCP stack time to flush the response
  scheduleReboot(1500);
  Serial.println("Credentials saved. Rebooting in 1.5s to test connection...");
}

static void handleStatus() {
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

static void handleLedStatus() {
  String json = "{\"led\":";
  json += ledState ? "true" : "false";
  json += "}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

static void handleLedToggle() {
  ledState = !ledState;
  digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
  setLedsEnabled(ledState);

  Serial.print("LED toggled: ");
  Serial.println(ledState ? "ON" : "OFF");

  String json = "{\"led\":";
  json += ledState ? "true" : "false";
  json += "}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

static void handleScan() {
  Serial.println("WiFi scan requested...");

  int numNetworks = WiFi.scanNetworks();
  Serial.print("Scan complete. Found ");
  Serial.print(numNetworks);
  Serial.println(" networks.");

  String json = "{\"networks\":[";
  bool firstNetwork = true;
  String addedSSIDs[50];
  int addedCount = 0;

  for (int i = 0; i < numNetworks && i < 50; i++) {
    String ssid = WiFi.SSID(i);
    int    rssi = WiFi.RSSI(i);

    if (ssid.length() == 0) continue;  // Skip hidden networks

    bool isDuplicate = false;
    for (int j = 0; j < addedCount; j++) {
      if (addedSSIDs[j] == ssid) { isDuplicate = true; break; }
    }
    if (isDuplicate) continue;

    if (!firstNetwork) json += ",";
    json += "{\"ssid\":\"";
    json += ssid;
    json += "\",\"rssi\":";
    json += String(rssi);
    json += ",\"encryption\":";
    json += String(WiFi.encryptionType(i));
    json += "}";

    addedSSIDs[addedCount++] = ssid;
    firstNetwork = false;
  }

  json += "]}";

  Serial.print("Filtered to ");
  Serial.print(addedCount);
  Serial.println(" unique networks");

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
  WiFi.scanDelete();
}

static void handleNotFound() {
  // Redirect all unknown routes to root — helps with captive portal detection
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

void setupProvisioningRoutes() {
  server.on("/", handleProvisioningRoot);
  server.on("/fonts/Gluten.ttf",  handleGlutenFont);
  server.on("/fonts/Fredoka.ttf", handleFredokaFont);
  server.on("/api/scan",    handleScan);
  server.on("/api/connect", HTTP_POST, handleConnect);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("✓ Web server started on port 80");
  Serial.println("\n=================================");
  Serial.println("Connect to WiFi: Blossom_Setup");
  Serial.println("Open browser: http://192.168.4.1");
  Serial.println("=================================\n");
}

void setupConnectedRoutes() {
  server.on("/", handleConnectedRoot);
  server.on("/fonts/Gluten.ttf",  handleGlutenFont);
  server.on("/fonts/Fredoka.ttf", handleFredokaFont);
  server.on("/api/status", handleStatus);
  server.on("/api/led", HTTP_GET, handleLedStatus);
  server.on("/api/led/toggle", HTTP_POST, handleLedToggle);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("✓ Web server started (connected mode)");
}
