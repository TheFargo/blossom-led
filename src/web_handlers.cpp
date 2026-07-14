#include "web_handlers.h"
#include "credentials.h"
#include "led_controller.h"
#include "animation_config.h"
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

// ── JSON parsing helpers for BlossomConfig ────────────────────────────────────
static uint8_t parseUint8(const String& body, const char* key) {
  String search = String("\"") + key + "\":";
  int start = body.indexOf(search);
  if (start < 0) return 0;
  start += search.length();
  int end = start;
  while (end < body.length() && isdigit(body[end])) end++;
  return (uint8_t)body.substring(start, end).toInt();
}

static int8_t parseInt8(const String& body, const char* key) {
  String search = String("\"") + key + "\":";
  int start = body.indexOf(search);
  if (start < 0) return 0;
  start += search.length();
  int end = start;
  if (body[end] == '-') end++;
  while (end < body.length() && isdigit(body[end])) end++;
  return (int8_t)body.substring(start, end).toInt();
}

static bool parseBool(const String& body, const char* key) {
  String search = String("\"") + key + "\":";
  int start = body.indexOf(search);
  if (start < 0) return false;
  return body.substring(start).startsWith(search + "true");
}

static DistributionMode parseMode(const String& body, const char* key) {
  uint8_t val = parseUint8(body, key);
  // Valid range is now 0-3: Unison, Random, Ordered, Looping (see animation_config.h)
  if (val > 3) return DistributionMode::RANDOM;
  return (DistributionMode)val;
}


static void handleLeds() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"No data received\"}");
    return;
  }

  String body = server.arg("plain");
  
  // Parse "enabled" field from JSON
  bool enabled = parseBool(body, "enabled");
  
  ledState = enabled;
  digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
  setLedsEnabled(ledState);

  Serial.print("LEDs set: ");
  Serial.println(ledState ? "ON" : "OFF");

  String json = "{\"enabled\":";
  json += ledState ? "true" : "false";
  json += "}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

static void handleAnimation() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"No data received\"}");
    return;
  }

  String body = server.arg("plain");
  
  // Parse BlossomConfig from JSON
  BlossomConfig config;
  
  // Color settings
  config.color.primary    = parseUint8(body, "color.primary");
  config.color.spread     = parseUint8(body, "color.spread");
  config.color.brightness = parseUint8(body, "color.brightness");
  config.color.mode       = parseMode(body, "color.mode");
  
  // Sparkle settings
  config.sparkles.brightness = parseUint8(body, "sparkles.brightness");
  config.sparkles.spread     = parseUint8(body, "sparkles.spread");
  config.sparkles.mode       = parseMode(body, "sparkles.mode");
  
  // Flicker animation
  config.flicker.apply_to_color    = parseBool(body, "flicker.apply_to_color");
  config.flicker.apply_to_sparkles = parseBool(body, "flicker.apply_to_sparkles");
  config.flicker.speed             = parseUint8(body, "flicker.speed");
  config.flicker.amplitude         = parseUint8(body, "flicker.amplitude");
  config.flicker.mode              = parseMode(body, "flicker.mode");
  
  // Pulse animation
  config.pulse.apply_to_color    = parseBool(body, "pulse.apply_to_color");
  config.pulse.apply_to_sparkles = parseBool(body, "pulse.apply_to_sparkles");
  config.pulse.speed             = parseUint8(body, "pulse.speed");
  config.pulse.amplitude         = parseUint8(body, "pulse.amplitude");
  config.pulse.mode              = parseMode(body, "pulse.mode");
  
  // Spin animation
  config.spin.apply_to_color    = parseBool(body, "spin.apply_to_color");
  config.spin.apply_to_sparkles = parseBool(body, "spin.apply_to_sparkles");
  config.spin.speed             = parseInt8(body, "spin.speed");
  
  // Apply configuration to LED controller
  setAnimationConfig(config);
  
  Serial.println("Animation config updated:");
  Serial.print("  Color: primary=");
  Serial.print(config.color.primary);
  Serial.print(" spread=");
  Serial.print(config.color.spread);
  Serial.print(" brightness=");
  Serial.println(config.color.brightness);
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", "{\"status\":\"success\"}");
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

  //Serial.print("Filtered to ");
  //Serial.print(addedCount);
  //Serial.println(" unique networks");

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
  //Serial.println("✓ Web server started on port 80");
  //Serial.println("\n=================================");
  //Serial.println("Connect to WiFi: Blossom_Setup");
  //Serial.println("Open browser: http://192.168.4.1");
  //Serial.println("=================================\n");
}

void setupConnectedRoutes() {
  server.on("/", handleConnectedRoot);
  server.on("/fonts/Gluten.ttf",  handleGlutenFont);
  server.on("/fonts/Fredoka.ttf", handleFredokaFont);
  server.on("/api/status", handleStatus);
  server.on("/api/led", HTTP_GET, handleLedStatus);
  server.on("/api/led/toggle", HTTP_POST, handleLedToggle);
  server.on("/api/leds", HTTP_POST, handleLeds);
  server.on("/api/animation", HTTP_POST, handleAnimation);
  server.onNotFound(handleNotFound);
  server.begin();
  //Serial.println("✓ Web server started (connected mode)");
}
