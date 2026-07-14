#ifndef ANIMATION_CONFIG_H
#define ANIMATION_CONFIG_H

#include <stdint.h>

// ── Shared enum for distribution patterns ─────────────────────────────────────
// Used for Color Spread, Sparkle Spread, and Synchronicity options.
enum class DistributionMode : uint8_t {
    UNISON  = 0,  // All 16 pixels act in perfect concert — no variation across the ring
    RANDOM  = 1,  // Randomly distributed among the pixels
    ORDERED = 2,  // Placed in order from first pixel to last
    LOOPING = 3   // Split in two for seamless transitions/reflections
};


// ── Container 2: Color Channel Settings ───────────────────────────────────────
struct ColorSettings {
    uint8_t primary;           // 0-255, representing hue (0-359° mapped to byte)
    uint8_t spread;            // 0-255, representing color variation
    uint8_t brightness;        // 0-255, relative to LED_BRIGHTNESS hard cap
    DistributionMode mode;     // Random, Ordered, or Looping spread
};

// ── Container 3: White Channel (Sparkles) Settings ────────────────────────────
struct SparkleSettings {
    uint8_t brightness;        // 0-255, representing white LED brightness levels
    uint8_t spread;            // 0-255, representing brightness variation
    DistributionMode mode;     // Random, Ordered, or Looping spread
};

// ── Reusable struct for Container 4 (Flicker) and Container 5 (Pulse) ─────────
struct AnimationTrack {
    bool apply_to_color;       // Applies animation to the color pixels
    bool apply_to_sparkles;    // Applies animation to the white pixels
    uint8_t speed;             // 0-255, representing animation speed
    uint8_t amplitude;         // 0-255, representing noise/curve amplitude
    DistributionMode mode;     // Synchronicity: Random, Ordered, or Looping
};

// ── Container 6: Spin Animation Settings ──────────────────────────────────────
struct SpinSettings {
    bool apply_to_color;       // Applies spin to the color pixels
    bool apply_to_sparkles;    // Applies spin to the white pixels
    int8_t speed;              // -128 to 127, representing spin speed and direction
};

// ── Master Configuration Payload ──────────────────────────────────────────────
struct BlossomConfig {
    ColorSettings color;       // Base color parameters
    SparkleSettings sparkles;  // Base white pixel parameters
    AnimationTrack flicker;    // Randomized noise track parameters
    AnimationTrack pulse;      // Sine-wave track parameters
    SpinSettings spin;         // Directional shift parameters
};

// ── Default Configuration (Warm Flame preset) ─────────────────────────────────
inline BlossomConfig getDefaultConfig() {
    BlossomConfig config = {};
    
    // Warm orange-yellow flame colors
    config.color.primary = 21;      // ~30° hue (orange)
    config.color.spread = 25;       // subtle yellow-to-red variation
    config.color.brightness = 200;  // fairly bright
    config.color.mode = DistributionMode::RANDOM;
    
    // Soft white sparkles
    config.sparkles.brightness = 100;
    config.sparkles.spread = 80;
    config.sparkles.mode = DistributionMode::RANDOM;
    
    // Gentle flicker on both channels
    config.flicker.apply_to_color = true;
    config.flicker.apply_to_sparkles = true;
    config.flicker.speed = 60;
    config.flicker.amplitude = 120;
    config.flicker.mode = DistributionMode::RANDOM;
    
    // Slow pulse on color only
    config.pulse.apply_to_color = true;
    config.pulse.apply_to_sparkles = false;
    config.pulse.speed = 30;
    config.pulse.amplitude = 80;
    config.pulse.mode = DistributionMode::LOOPING;
    
    // No spin by default
    config.spin.apply_to_color = false;
    config.spin.apply_to_sparkles = false;
    config.spin.speed = 0;
    
    return config;
}

#endif // ANIMATION_CONFIG_H
