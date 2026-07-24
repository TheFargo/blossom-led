#pragma once
#include <Arduino.h>
#include "animation_config.h"

// ###########################################################################
// ##                     PRESETS.h - Animation Presets                     ##
// ###########################################################################
// Save, load, and list named animation presets on the LittleFS file system.
//
// STORAGE LAYOUT (LittleFS):
//   /presets/<safe-name>.json   One file per preset. The file holds the same
//                               flat JSON the web page sends to /api/animation,
//                               plus a "name" field with the display name.
//   /preset_default.txt         Holds the *file-safe* name of the preset that
//                               should start playing at boot. If this file is
//                               missing (fresh device), the built-in
//                               "Warm Flame" config from animation_config.h
//                               fills the role instead.
//
// The module also tracks "what is playing right now" on behalf of the web UI:
//   - currentPresetName  ("Warm Flame", "My Preset", or "Custom")
//   - a Core-0-side copy of the active BlossomConfig, so GET /api/animation
//     can report the running settings to a freshly loaded web page.

// ── Limits ────────────────────────────────────────────────────────────────────
static const int    MAX_PRESETS         = 16;   // Cap on saved preset files
static const size_t MAX_PRESET_NAME_LEN = 24;   // Cap on display-name length

// ── Now-playing state ─────────────────────────────────────────────────────────
// The display name of whatever is currently animating. Web handlers set this
// to "Custom" whenever the user tweaks a control by hand.
extern String currentPresetName;

// ── Boot-time entry point (called from setup() on Core 0) ────────────────────
// Reads /preset_default.txt, loads the matching preset file, and pushes its
// config to the LED controller. Falls back to the built-in Warm Flame config
// if no default has been saved yet. Always leaves currentPresetName valid.
void applyDefaultPreset();

// ── Preset operations (called from web handlers on Core 0) ───────────────────
// Save `config` under `name`. Overwrites an existing preset with the same
// (sanitized) name. If makeDefault is true, this preset will play at boot.
bool savePreset(const String& name, const BlossomConfig& config, bool makeDefault);

// Load the preset called `name`, apply it to the LEDs, and return its stored
// JSON in `jsonOut` (so the web page can sync its controls). Returns false if
// no such preset exists. The built-in "Warm Flame" always loads successfully,
// even on a fresh device with no saved files.
bool loadPreset(const String& name, String& jsonOut);

// Build the JSON for GET /api/presets:
//   {"current":"...","default":"...","presets":["Warm Flame","..."]}
String getPresetListJson();

// ── Active-config bookkeeping ─────────────────────────────────────────────────
// Apply a config to the LED controller *and* remember it on Core 0 so the web
// page can read the running settings back. All web handlers should use this
// instead of calling setAnimationConfig() directly.
void applyActiveConfig(const BlossomConfig& config, const String& name);

// The flat JSON of the currently running config plus the now-playing name —
// the body served by GET /api/animation.
String getActiveConfigJson();

// ── Shared JSON helpers ───────────────────────────────────────────────────────
// The project deliberately avoids a JSON library: the payloads are small, flat
// objects with known keys, so a few string-scanning helpers are all we need.
// These are used by both the animation handler and the preset code.
uint8_t  jsonParseUint8 (const String& body, const char* key);
int8_t   jsonParseInt8  (const String& body, const char* key);
bool     jsonParseBool  (const String& body, const char* key);
String   jsonParseString(const String& body, const char* key);

// Parse a full flat-key config object ("color.primary", "flicker.speed", ...)
// into a BlossomConfig, and serialize one back out. These two functions are
// inverses of each other and define the on-disk preset format.
BlossomConfig parseConfigFromJson(const String& body);
String        configToJson(const BlossomConfig& config);