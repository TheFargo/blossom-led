#include "web_handlers.h"
#include "credentials.h"
#include "led_controller.h"
#include "animation_config.h"
#include "presets.h"
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

static void handleMeditationRoot() {
  serveFile("/meditation.html", "text/html");
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

// NOTE: The flat-JSON parsing helpers (jsonParseUint8, jsonParseBool, etc.)
// used to live here, but presets.cpp needs them too — they moved to the
// presets module (see presets.h) so both files share one implementation.

static void handleLeds() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"No data received\"}");
    return;
  }

  String body = server.arg("plain");
  
  // Parse "enabled" field from JSON
  bool enabled = jsonParseBool(body, "enabled");
  
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

  // Parse the flat-key config JSON into a BlossomConfig (shared parser in
  // presets.cpp) and apply it. A hand-tweaked config is by definition not a
  // named preset anymore, so the "Now Playing" title becomes "Custom".
  BlossomConfig config = parseConfigFromJson(body);
  applyActiveConfig(config, "Custom");

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

// GET /api/animation — report the currently running config (plus the
// now-playing name) so a freshly opened web page can sync its controls to
// what's actually on the ring instead of assuming hard-coded defaults.
static void handleAnimationGet() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", getActiveConfigJson());
}

// ── Preset endpoints ──────────────────────────────────────────────────────────

// GET /api/presets — list all saved presets, plus which one is playing and
// which one is the boot default. The web page uses this to build the
// scrap-paper preset picker.
static void handlePresetList() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", getPresetListJson());
}

// POST /api/presets/save — body: {"name":"...","makeDefault":true, ...config}
// Saves the supplied config under the given name (overwriting any preset with
// the same name) and optionally marks it as the boot default.
static void handlePresetSave() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"No data received\"}");
    return;
  }

  String body        = server.arg("plain");
  String name        = jsonParseString(body, "name");
  bool   makeDefault = jsonParseBool(body, "makeDefault");
  BlossomConfig config = parseConfigFromJson(body);

  server.sendHeader("Access-Control-Allow-Origin", "*");
  if (savePreset(name, config, makeDefault)) {
    // Echo back the (possibly cleaned-up) name now shown as "Now Playing"
    String json = "{\"status\":\"saved\",\"name\":\"";
    json += currentPresetName;
    json += "\"}";
    server.send(200, "application/json", json);
  } else {
    server.send(400, "application/json", "{\"error\":\"Could not save preset\"}");
  }
}

// POST /api/presets/load — body: {"name":"..."}
// Applies the named preset to the LEDs and returns its full config JSON so
// the web page can update every slider/radio/checkbox to match.
static void handlePresetLoad() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"No data received\"}");
    return;
  }

  String name = jsonParseString(server.arg("plain"), "name");
  String presetJson;

  server.sendHeader("Access-Control-Allow-Origin", "*");
  if (loadPreset(name, presetJson)) {
    server.send(200, "application/json", presetJson);
  } else {
    server.send(404, "application/json", "{\"error\":\"Preset not found\"}");
  }
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

// ── Meditation Mode endpoints ──────────────────────────────────────────────────

// POST /api/meditation/start — body: {"duration": 0|30|60} (0 = open-ended)
static void handleMeditationStart() {
  uint32_t duration = 0;
  if (server.hasArg("plain")) {
    duration = (uint32_t)jsonParseUint8(server.arg("plain"), "duration");
    // jsonParseUint8 caps at 255, which comfortably covers 0/30/60; if a
    // future duration option exceeds a byte, switch to a wider parser.
  }

  startMeditation(duration);

  Serial.print("Meditation started, duration=");
  Serial.println(duration);

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", "{\"status\":\"started\"}");
}

// POST /api/meditation/stop — ends the session immediately (no ending blink)
static void handleMeditationStop() {
  stopMeditation();
  Serial.println("Meditation stopped");

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", "{\"status\":\"stopped\"}");
}

// GET /api/meditation/status — polled by meditation.html at a steady clip to
// drive the on-screen instructions ("Inhale", "Hold", "Exhale"...) in sync
// with the LEDs.
static void handleMeditationStatus() {
  MeditationStatus status = getMeditationStatus();

  String json = "{";
  json += "\"active\":";
  json += status.active ? "true" : "false";
  json += ",\"phase\":\"";
  json += meditationPhaseToString(status.phase);
  json += "\",\"phaseSecondsLeft\":";
  json += String(status.phaseSecondsLeft);
  json += ",\"sessionSecondsElapsed\":";
  json += String(status.sessionSecondsElapsed);
  json += ",\"sessionSecondsRemaining\":";
  json += String(status.sessionSecondsRemaining);
  json += "}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
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
  server.on("/meditation.html", handleMeditationRoot);
  server.on("/fonts/Gluten.ttf",  handleGlutenFont);
  server.on("/fonts/Fredoka.ttf", handleFredokaFont);
  server.on("/api/status", handleStatus);
  server.on("/api/led", HTTP_GET, handleLedStatus);
  server.on("/api/led/toggle", HTTP_POST, handleLedToggle);
  server.on("/api/leds", HTTP_POST, handleLeds);
  server.on("/api/animation", HTTP_GET,  handleAnimationGet);
  server.on("/api/animation", HTTP_POST, handleAnimation);
  server.on("/api/presets",      HTTP_GET,  handlePresetList);
  server.on("/api/presets/save", HTTP_POST, handlePresetSave);
  server.on("/api/presets/load", HTTP_POST, handlePresetLoad);
  server.on("/api/meditation/start",  HTTP_POST, handleMeditationStart);
  server.on("/api/meditation/stop",   HTTP_POST, handleMeditationStop);
  server.on("/api/meditation/status", HTTP_GET,  handleMeditationStatus);
  server.onNotFound(handleNotFound);
  server.begin();
  //Serial.println("✓ Web server started (connected mode)");
}
