// ###########################################################################
// ##                    PRESETS.cpp - Animation Presets                    ##
// ###########################################################################
// Everything preset-related lives here: the shared flat-JSON helpers, the
// LittleFS save/load/list operations, and the "now playing" bookkeeping the
// web UI relies on. See presets.h for the storage layout overview.
//
// DESIGN NOTES
// - No JSON library. The payloads are tiny, flat, and have a fixed set of
//   known keys, so simple String scanning is smaller and easier to follow
//   than pulling in ArduinoJson. parseConfigFromJson()/configToJson() are
//   exact inverses, which makes them double as the on-disk preset format.
// - Everything here runs on Core 0 (web handlers + setup()). Core 1 never
//   touches the file system — the only cross-core traffic is the existing
//   setAnimationConfig() hand-off in led_controller.cpp.
// - Preset display names can contain spaces and punctuation, but file names
//   cannot, so each preset gets a "sanitized" file-safe name (lowercase
//   alphanumerics and underscores). The pretty display name is stored *inside*
//   the file under a "name" key. Two display names that sanitize identically
//   ("My Preset!" and "my preset") intentionally overwrite each other.

#include "presets.h"
#include "led_controller.h"
#include <LittleFS.h>

// ── Storage locations ─────────────────────────────────────────────────────────
static const char* PRESET_DIR    = "/presets";            // one .json file per preset
static const char* DEFAULT_FILE  = "/preset_default.txt"; // file-safe name of the boot preset

// The built-in preset that ships with every Blossom. It has no file on disk —
// getDefaultConfig() in animation_config.h *is* its definition — but it always
// appears in the preset list and can always be loaded, even on a fresh device.
static const char* BUILTIN_PRESET_NAME = "Warm Flame";

// ── Now-playing state (Core 0's view of the world) ────────────────────────────
String currentPresetName = BUILTIN_PRESET_NAME;

// Core-0-side copy of whatever config was last pushed to the LED controller.
// Core 1 keeps its own copy inside led_controller.cpp; this one exists purely
// so GET /api/animation can tell a freshly opened web page what's playing.
static BlossomConfig activeConfig = getDefaultConfig();

// ###########################################################################
// ##                        Shared JSON helpers                            ##
// ###########################################################################
// Each helper scans the body for `"key":` and reads the value that follows.
// The keys we use contain dots ("color.primary"), which conveniently makes
// them unambiguous — no key is a prefix of another key's *quoted* form.

uint8_t jsonParseUint8(const String& body, const char* key) {
  String search = String("\"") + key + "\":";
  int start = body.indexOf(search);
  if (start < 0) return 0;
  start += search.length();
  int end = start;
  while (end < (int)body.length() && isdigit(body[end])) end++;
  return (uint8_t)body.substring(start, end).toInt();
}

int8_t jsonParseInt8(const String& body, const char* key) {
  String search = String("\"") + key + "\":";
  int start = body.indexOf(search);
  if (start < 0) return 0;
  start += search.length();
  int end = start;
  if (body[end] == '-') end++;  // allow a leading minus sign (spin can go negative)
  while (end < (int)body.length() && isdigit(body[end])) end++;
  return (int8_t)body.substring(start, end).toInt();
}

bool jsonParseBool(const String& body, const char* key) {
  String search = String("\"") + key + "\":";
  int start = body.indexOf(search);
  if (start < 0) return false;
  return body.substring(start).startsWith(search + "true");
}

// Extract a quoted string value: "name":"My Preset" → My Preset
// Returns an empty String if the key is missing.
String jsonParseString(const String& body, const char* key) {
  String search = String("\"") + key + "\":\"";
  int start = body.indexOf(search);
  if (start < 0) return String("");
  start += search.length();
  int end = body.indexOf('"', start);
  if (end < 0) return String("");
  return body.substring(start, end);
}

// DistributionMode arrives as an integer 0-3 (see animation_config.h).
// Anything out of range falls back to RANDOM, the most forgiving default.
static DistributionMode jsonParseMode(const String& body, const char* key) {
  uint8_t val = jsonParseUint8(body, key);
  if (val > 3) return DistributionMode::RANDOM;
  return (DistributionMode)val;
}

// ── Full-config parse / serialize (exact inverses of each other) ──────────────

BlossomConfig parseConfigFromJson(const String& body) {
  BlossomConfig config;

  // Color settings
  config.color.primary    = jsonParseUint8(body, "color.primary");
  config.color.spread     = jsonParseUint8(body, "color.spread");
  config.color.brightness = jsonParseUint8(body, "color.brightness");
  config.color.mode       = jsonParseMode (body, "color.mode");

  // Sparkle settings
  config.sparkles.brightness = jsonParseUint8(body, "sparkles.brightness");
  config.sparkles.spread     = jsonParseUint8(body, "sparkles.spread");
  config.sparkles.mode       = jsonParseMode (body, "sparkles.mode");

  // Flicker animation
  config.flicker.apply_to_color    = jsonParseBool (body, "flicker.apply_to_color");
  config.flicker.apply_to_sparkles = jsonParseBool (body, "flicker.apply_to_sparkles");
  config.flicker.speed             = jsonParseUint8(body, "flicker.speed");
  config.flicker.amplitude         = jsonParseUint8(body, "flicker.amplitude");
  config.flicker.mode              = jsonParseMode (body, "flicker.mode");

  // Pulse animation
  config.pulse.apply_to_color    = jsonParseBool (body, "pulse.apply_to_color");
  config.pulse.apply_to_sparkles = jsonParseBool (body, "pulse.apply_to_sparkles");
  config.pulse.speed             = jsonParseUint8(body, "pulse.speed");
  config.pulse.amplitude         = jsonParseUint8(body, "pulse.amplitude");
  config.pulse.mode              = jsonParseMode (body, "pulse.mode");

  // Spin animation
  config.spin.apply_to_color    = jsonParseBool(body, "spin.apply_to_color");
  config.spin.apply_to_sparkles = jsonParseBool(body, "spin.apply_to_sparkles");
  config.spin.speed             = jsonParseInt8(body, "spin.speed");

  return config;
}

// Small append helpers keep configToJson() readable and sidestep the
// const char* + String concatenation pitfall (see CLAUDE.md).
static void appendNum(String& json, const char* key, int value, bool comma = true) {
  json += "\"";
  json += key;
  json += "\":";
  json += String(value);
  if (comma) json += ",";
}

static void appendBool(String& json, const char* key, bool value, bool comma = true) {
  json += "\"";
  json += key;
  json += "\":";
  json += value ? "true" : "false";
  if (comma) json += ",";
}

String configToJson(const BlossomConfig& config) {
  String json = "{";
  appendNum (json, "color.primary",             config.color.primary);
  appendNum (json, "color.spread",              config.color.spread);
  appendNum (json, "color.brightness",          config.color.brightness);
  appendNum (json, "color.mode",                (int)config.color.mode);
  appendNum (json, "sparkles.brightness",       config.sparkles.brightness);
  appendNum (json, "sparkles.spread",           config.sparkles.spread);
  appendNum (json, "sparkles.mode",             (int)config.sparkles.mode);
  appendBool(json, "flicker.apply_to_color",    config.flicker.apply_to_color);
  appendBool(json, "flicker.apply_to_sparkles", config.flicker.apply_to_sparkles);
  appendNum (json, "flicker.speed",             config.flicker.speed);
  appendNum (json, "flicker.amplitude",         config.flicker.amplitude);
  appendNum (json, "flicker.mode",              (int)config.flicker.mode);
  appendBool(json, "pulse.apply_to_color",      config.pulse.apply_to_color);
  appendBool(json, "pulse.apply_to_sparkles",   config.pulse.apply_to_sparkles);
  appendNum (json, "pulse.speed",               config.pulse.speed);
  appendNum (json, "pulse.amplitude",           config.pulse.amplitude);
  appendNum (json, "pulse.mode",                (int)config.pulse.mode);
  appendBool(json, "spin.apply_to_color",       config.spin.apply_to_color);
  appendBool(json, "spin.apply_to_sparkles",    config.spin.apply_to_sparkles);
  appendNum (json, "spin.speed",                config.spin.speed, false);  // last field: no trailing comma
  json += "}";
  return json;
}

// ###########################################################################
// ##                        Name handling helpers                          ##
// ###########################################################################

// Trim, strip characters that would break our hand-rolled JSON (quotes and
// backslashes), and enforce the length cap. This is the *display* name.
static String cleanDisplayName(const String& raw) {
  String name = raw;
  name.trim();
  String out = "";
  for (size_t i = 0; i < name.length() && out.length() < MAX_PRESET_NAME_LEN; i++) {
    char c = name[i];
    if (c == '"' || c == '\\' || c < 32) continue;  // drop JSON-hostile characters
    out += c;
  }
  return out;
}

// Convert a display name to a file-safe name: lowercase alphanumerics, with
// every run of anything else collapsed to a single underscore.
// "Warm Flame" → "warm_flame". This is what appears in the /presets/ path.
static String sanitizeFileName(const String& displayName) {
  String out = "";
  bool lastWasUnderscore = false;
  for (size_t i = 0; i < displayName.length(); i++) {
    char c = displayName[i];
    if (isalnum(c)) {
      out += (char)tolower(c);
      lastWasUnderscore = false;
    } else if (!lastWasUnderscore && out.length() > 0) {
      out += '_';
      lastWasUnderscore = true;
    }
  }
  // Trim a trailing underscore left by e.g. "My Preset!"
  while (out.length() > 0 && out[out.length() - 1] == '_') {
    out.remove(out.length() - 1);
  }
  return out;
}

// Full LittleFS path for a preset's file-safe name
static String presetPath(const String& safeName) {
  String path = PRESET_DIR;
  path += "/";
  path += safeName;
  path += ".json";
  return path;
}

// Read an entire (small) preset file into a String. Returns "" on failure.
static String readPresetFile(const String& safeName) {
  File file = LittleFS.open(presetPath(safeName), "r");
  if (!file) return String("");
  String content = file.readString();
  file.close();
  return content;
}

// Count how many preset files exist (used to enforce MAX_PRESETS)
static int countPresetFiles() {
  int count = 0;
  Dir dir = LittleFS.openDir(PRESET_DIR);
  while (dir.next()) {
    if (dir.isFile()) count++;
  }
  return count;
}

// ###########################################################################
// ##                      Active-config bookkeeping                        ##
// ###########################################################################

void applyActiveConfig(const BlossomConfig& config, const String& name) {
  activeConfig      = config;          // Core 0's reference copy (for GET /api/animation)
  currentPresetName = name;            // What the "Now Playing" button should show
  setAnimationConfig(config);          // Hand the config across to Core 1's render loop
}

String getActiveConfigJson() {
  // Same flat keys the web page sends, plus the now-playing name so the page
  // can initialize its controls *and* its title from one request.
  String json = configToJson(activeConfig);
  // Splice the name in right after the opening brace: {"name":"...", ...}
  String withName = "{\"name\":\"";
  withName += currentPresetName;
  withName += "\",";
  withName += json.substring(1);  // skip configToJson's own "{"
  return withName;
}

// ###########################################################################
// ##                          Preset operations                            ##
// ###########################################################################

bool savePreset(const String& rawName, const BlossomConfig& config, bool makeDefault) {
  String displayName = cleanDisplayName(rawName);
  String safeName    = sanitizeFileName(displayName);
  if (safeName.length() == 0) {
    Serial.println("x Preset save rejected: empty or invalid name");
    return false;
  }

  // Enforce the preset cap — but overwriting an existing preset is always OK
  bool exists = LittleFS.exists(presetPath(safeName));
  if (!exists && countPresetFiles() >= MAX_PRESETS) {
    Serial.println("x Preset save rejected: preset limit reached");
    return false;
  }

  LittleFS.mkdir(PRESET_DIR);  // no-op if the directory already exists

  File file = LittleFS.open(presetPath(safeName), "w");
  if (!file) {
    Serial.println("x Failed to open preset file for writing");
    return false;
  }

  // File format: the canonical config JSON with the display name spliced in.
  // Storing the *parsed-then-reserialized* config (rather than the raw HTTP
  // body) means anything we write to disk is guaranteed well-formed.
  String json = "{\"name\":\"";
  json += displayName;
  json += "\",";
  json += configToJson(config).substring(1);  // skip the opening "{"
  file.print(json);
  file.close();

  // "Make Default": remember this preset's file-safe name for the next boot
  if (makeDefault) {
    File def = LittleFS.open(DEFAULT_FILE, "w");
    if (def) {
      def.print(safeName);
      def.close();
      Serial.print("+ Default boot preset set to: ");
      Serial.println(displayName);
    }
  }

  // The just-saved settings are what's on the ring right now, so the saved
  // preset immediately becomes the "Now Playing" title (replacing "Custom").
  applyActiveConfig(config, displayName);

  Serial.print("+ Preset saved: ");
  Serial.println(displayName);
  return true;
}

bool loadPreset(const String& rawName, String& jsonOut) {
  String safeName = sanitizeFileName(cleanDisplayName(rawName));

  // Try a saved file first — a user preset named "Warm Flame" (unlikely but
  // legal) takes precedence over the built-in.
  String content = readPresetFile(safeName);
  if (content.length() > 0) {
    String displayName = jsonParseString(content, "name");
    if (displayName.length() == 0) displayName = rawName;
    applyActiveConfig(parseConfigFromJson(content), displayName);
    jsonOut = content;
    Serial.print("+ Preset loaded: ");
    Serial.println(displayName);
    return true;
  }

  // No file? The built-in Warm Flame preset lives in code, not on disk.
  if (safeName == sanitizeFileName(BUILTIN_PRESET_NAME)) {
    BlossomConfig config = getDefaultConfig();
    applyActiveConfig(config, BUILTIN_PRESET_NAME);
    jsonOut = getActiveConfigJson();  // synthesized JSON, same shape as a file
    Serial.println("+ Built-in preset loaded: Warm Flame");
    return true;
  }

  Serial.print("x Preset not found: ");
  Serial.println(rawName);
  return false;
}

String getPresetListJson() {
  // {"current":"...","default":"...","presets":["Warm Flame","My Preset",...]}
  // The built-in Warm Flame is always listed first; saved presets follow in
  // whatever order the file system yields them.

  // Resolve the default preset's display name (falls back to the built-in)
  String defaultDisplay = BUILTIN_PRESET_NAME;
  File def = LittleFS.open(DEFAULT_FILE, "r");
  if (def) {
    String safeName = def.readString();
    def.close();
    safeName.trim();
    String content = readPresetFile(safeName);
    if (content.length() > 0) {
      String name = jsonParseString(content, "name");
      if (name.length() > 0) defaultDisplay = name;
    }
  }

  String json = "{\"current\":\"";
  json += currentPresetName;
  json += "\",\"default\":\"";
  json += defaultDisplay;
  json += "\",\"presets\":[\"";
  json += BUILTIN_PRESET_NAME;
  json += "\"";

  // Append each saved preset's display name (read from inside its file).
  // Skip any file that shadows the built-in name to avoid a duplicate entry.
  String builtinSafe = sanitizeFileName(BUILTIN_PRESET_NAME);
  Dir dir = LittleFS.openDir(PRESET_DIR);
  while (dir.next()) {
    if (!dir.isFile()) continue;
    File file = dir.openFile("r");
    if (!file) continue;
    String content = file.readString();
    file.close();
    String name = jsonParseString(content, "name");
    if (name.length() == 0) continue;
    if (sanitizeFileName(name) == builtinSafe) continue;
    json += ",\"";
    json += name;
    json += "\"";
  }

  json += "]}";
  return json;
}

// ###########################################################################
// ##                        Boot-time default preset                       ##
// ###########################################################################

void applyDefaultPreset() {
  // Called once from setup() on Core 0, right after LittleFS mounts and
  // *before* the (potentially slow) WiFi connection attempt — so the boot
  // animation starts immediately, connected or not.
  //
  // Note on cross-core timing: Core 1 initializes its own built-in Warm Flame
  // config inside initLEDs(), but only if no config has arrived yet — see the
  // _configReceived guard in led_controller.cpp. That makes this call safe
  // regardless of which core wins the boot race.
  File def = LittleFS.open(DEFAULT_FILE, "r");
  if (def) {
    String safeName = def.readString();
    def.close();
    safeName.trim();
    String content = readPresetFile(safeName);
    if (content.length() > 0) {
      String displayName = jsonParseString(content, "name");
      if (displayName.length() == 0) displayName = safeName;
      applyActiveConfig(parseConfigFromJson(content), displayName);
      Serial.print("+ Boot preset applied: ");
      Serial.println(displayName);
      return;
    }
    Serial.println("! Default preset file missing - falling back to Warm Flame");
  }

  // Fresh device (or missing file): the built-in Warm Flame fills the role.
  // No need to push a config — Core 1 already boots with it — just make sure
  // Core 0's bookkeeping agrees.
  activeConfig      = getDefaultConfig();
  currentPresetName = BUILTIN_PRESET_NAME;
  Serial.println("+ Boot preset: built-in Warm Flame");
}