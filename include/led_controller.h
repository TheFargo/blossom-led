#pragma once
#include <Arduino.h>
#include "animation_config.h"

// LED Controller — SK6812 RGBW NeoPixel animation engine (PIO direct-drive)
// Runs on Core 1 via setup1() / loop1() (Earle Philhower dual-core)

void initLEDs();
void setLedsEnabled(bool enabled);
void setEffect(const char* name, uint32_t color, float speed, unsigned long duration);
void setAnimationConfig(const BlossomConfig& config);
void updateLEDs();

// ── Meditation Mode ─────────────────────────────────────────────────────────
// Box-breathing (4s inhale / 4s hold / 4s exhale / 4s hold) guided session.
// Runs entirely on Core 1 (renderMeditationFrame(), called from updateLEDs())
// so timing stays smooth regardless of what Core 0 / the web server is doing.
// Core 0 sends start/stop commands via a small inter-core queue (same pattern
// as setLedsEnabled()) and polls getMeditationStatus() for the web page.
enum class MeditationPhase : uint8_t {
    IDLE       = 0,  // not meditating — normal animation is playing
    INHALE     = 1,
    HOLD_FULL  = 2,
    EXHALE     = 3,
    HOLD_EMPTY = 4,
    ENDING     = 5   // five friendly white blinks, then back to IDLE
};

struct MeditationStatus {
    bool     active;                    // false once the session has fully ended
    MeditationPhase phase;
    uint32_t phaseSecondsLeft;           // seconds left in the current phase (~1-4)
    uint32_t sessionSecondsElapsed;
    int32_t  sessionSecondsRemaining;    // -1 = open-ended session
};

void startMeditation(uint32_t durationSeconds);  // 0 = open-ended
void stopMeditation();
MeditationStatus getMeditationStatus();
const char* meditationPhaseToString(MeditationPhase phase);


