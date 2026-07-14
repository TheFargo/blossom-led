// DATA STRUCTURE for BLOSSOM LIGHT RINGS
// Used for creating flexible ambient repeating light patterns
// controlling both colors and animations.
// The Blossom's LEDs have three color channels and a special "warm white" LED channel.
// These can be controlled independantly as the "color" and "sparkle" lights.
// Additional details (and the UI for interfacing with this data) are in CONTROLS.md.

#include <stdint.h>

// Shared enum for the Radio Button selections across containers.
// Covers Color Spread, Sparkle Spread, and Synchronicity options.
enum class DistributionMode : uint8_t {
    UNISON = 0,  // All pixels act in perfect concert (no variation across the ring)
    RANDOM = 1,  // Randomly distributed among the pixels
    ORDERED = 2, // Placed in order from first pixel to last
    LOOPING = 3  // Split in two for seamless transitions/reflections
};


// Container 2: Color Channel Settings
struct ColorSettings {
    uint8_t primary;           // 255 positions, representing hue
    uint8_t spread;            // 255 positions, representing color variation
    uint8_t brightness;        // 255 positions, relative to LED_BRIGHTNESS hard cap
    DistributionMode mode;     // Random, Ordered, or Looping spread
};

// Container 3: White Channel (Sparkles) Settings
struct SparkleSettings {
    uint8_t brightness;        // 255 positions, representing white LED brightness levels
    uint8_t spread;            // 255 positions, representing brightness variation
    DistributionMode mode;     // Random, Ordered, or Looping spread
};

// Reusable struct for Container 4 (Flicker) and Container 5 (Pulse)
struct AnimationTrack {
    bool apply_to_color;       // Applies animation to the color pixels
    bool apply_to_sparkles;    // Applies animation to the white pixels
    uint8_t speed;             // 255 positions, representing animation speed
    uint8_t amplitude;         // 255 positions, representing noise/curve amplitude
    DistributionMode mode;     // Synchronicity: Random, Ordered, or Looping
};

// Container 6: Spin Animation Settings
struct SpinSettings {
    bool apply_to_color;       // Applies spin to the color pixels
    bool apply_to_sparkles;    // Applies spin to the white pixels
    int8_t speed;              // -128 to 128, representing spin speed and direction
};

// Master Configuration Payload
struct BlossomConfig {
    ColorSettings color;       // Base color parameters
    SparkleSettings sparkles;  // Base white pixel parameters
    AnimationTrack flicker;    // Randomized noise track parameters
    AnimationTrack pulse;      // Sine-wave track parameters
    SpinSettings spin;         // Directional shift parameters
};