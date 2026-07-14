#include "led_controller.h"
#include "animation_config.h"
#include <Arduino.h>
#include <cmath>
#include <hardware/pio.h>
#include <hardware/clocks.h>
#include <pico/util/queue.h>

// ── Hardware configuration ─────────────────────────────────────────────────────
#define LED_PIN    16   // GP16 – NeoPixel data line (via logic-level shifter)
#define LED_COUNT  16   // 16-pixel SK6812 RGBW ring

// Master brightness scalar (0.0 – 1.0). Adjust to taste.
static const float LED_BRIGHTNESS = 0.1f;

// ── PIO program (SK6812/WS2812 bit-banging, 8 MHz clock) ──────────────────────
// Translated from the MicroPython neopixel_projector PIO assembly.
// Timings: T1=2, T2=5, T3=3 (each cycle = 125 ns at 8 MHz)
//   "1" bit: LOW 3 cycles (375 ns) | HIGH 7 cycles (875 ns)
//   "0" bit: LOW 8 cycles (1 µs)   | HIGH 2 cycles (250 ns)
// .side_set 1 — bit 12 of each instruction word is the sideset pin value.
static const uint16_t ws2812_program_instructions[] = {
    //        Instruction                side  delay
    0x6221u,  // out x, 1               0     2     (pin LOW  for T3 = 3 cycles)
    0x1123u,  // jmp !x, 3  → do_zero   1     1     (pin HIGH for T1 = 2 cycles; !x = condition 001)
    0x1400u,  // jmp 0  (do_one→bitloop)1     4     (pin HIGH for T2 = 5 cycles)
    0xa442u,  // mov y, y (nop/do_zero) 0     4     (pin LOW  for T2 = 5 cycles)
};
static const pio_program_t ws2812_program = {
    .instructions = ws2812_program_instructions,
    .length       = 4,
    .origin       = -1,
};

// ── PIO state ──────────────────────────────────────────────────────────────────
static PIO  _pio = pio0;
static uint _sm  = 0;

// ── Inter-core command queue ───────────────────────────────────────────────────
// Core 0 calls setLedsEnabled() → pushes a uint8_t (0=off, 1=on).
// Core 1 pops it inside updateLEDs().
static queue_t _commandQueue;
static bool    _ledsEnabled = true;  // Core 1 local state

// ── Shared animation configuration ────────────────────────────────────────────
// Core 0 writes via setAnimationConfig(), Core 1 reads in updateLEDs().
// Simple copy-on-read, no mutex needed (single writer, single reader).
static BlossomConfig _currentConfig;
static volatile bool _configUpdated = false;

// ── PIO state machine initialisation ──────────────────────────────────────────
static void ws2812_sm_init(PIO pio, uint sm, uint offset, uint pin) {
    pio_gpio_init(pio, pin);
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);  // pin is output

    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset, offset + 3);           // wrap_target=0, wrap=3
    sm_config_set_sideset_pins(&c, pin);
    sm_config_set_sideset(&c, 1, false, false);           // 1 sideset bit, not optional
    sm_config_set_out_shift(&c, false, true, 32);         // shift left, autopull at 32 bits
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);        // 8-word TX FIFO

    // 8 MHz for WS2812-compatible timing (matches Python reference)
    float div = (float)clock_get_hz(clk_sys) / 8000000.0f;
    sm_config_set_clkdiv(&c, div);

    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

// ── Pixel helpers ──────────────────────────────────────────────────────────────
static inline void put_pixel(uint32_t grbw) {
    pio_sm_put_blocking(_pio, _sm, grbw);
}

// hue 0-359, sat/val 0-255 → packed SK6812 GRBW word (W=0 for saturated colours)
static uint32_t hsv_to_grbw(uint16_t hue, uint8_t sat, uint8_t val) {
    uint8_t region = hue / 60u;
    uint8_t rem    = (uint8_t)((hue % 60u) * 255u / 60u);
    uint8_t p = (uint8_t)((uint32_t)val * (255u - sat) / 255u);
    uint8_t q = (uint8_t)((uint32_t)val * (255u - (uint32_t)sat * rem  / 255u) / 255u);
    uint8_t t = (uint8_t)((uint32_t)val * (255u - (uint32_t)sat * (255u - rem) / 255u) / 255u);
    uint8_t r, g, b;
    switch (region) {
        case 0:  r = val; g = t;   b = p;   break;
        case 1:  r = q;   g = val; b = p;   break;
        case 2:  r = p;   g = val; b = t;   break;
        case 3:  r = p;   g = q;   b = val; break;
        case 4:  r = t;   g = p;   b = val; break;
        default: r = val; g = p;   b = q;   break;
    }
    // SK6812 wire order: G, R, B, W (MSB first, matched by left-shift autopull)
    return ((uint32_t)g << 24) | ((uint32_t)r << 16) | ((uint32_t)b << 8) | 0u;
}

static void send_reset() {
    delayMicroseconds(100);  // SK6812 requires ≥80 µs LOW between frames
}

static void send_off_frame() {
    for (int i = 0; i < LED_COUNT; i++) put_pixel(0u);
    send_reset();
}

static void send_rainbow_frame(uint16_t phase) {
    uint8_t val = (uint8_t)(255.0f * LED_BRIGHTNESS);
    for (int i = 0; i < LED_COUNT; i++) {
        uint16_t hue = (uint16_t)(((uint32_t)i * 360u / LED_COUNT + phase) % 360u);
        put_pixel(hsv_to_grbw(hue, 255u, val));
    }
    send_reset();
}

// ── Animation math helpers ──────────────────────────────────────────────────────
// These functions generate the time-varying "signal" that Flicker and Pulse use
// to move a pixel between the two extremes of its spread. Both return a value in
// the range [-1, +1] — the caller scales that by amplitude and by (spread / 2)
// to get an actual hue/brightness offset. Keeping the return range normalized
// means Flicker and Pulse are interchangeable/stackable: the render loop doesn't
// need to know which track produced the number, just how far it swings.

// ── Flicker: deterministic "value noise" ──────────────────────────────────────
// True random flicker looks harsh and strobe-y frame to frame. Instead we use
// "value noise": pick a pseudo-random target value at fixed time intervals and
// smoothly interpolate ("smoothstep") between the previous and next target.
// This is entirely stateless — no per-pixel arrays to maintain — because the
// "random" keyframes are generated on demand from a hash of (seed, time-bucket).
// Feed it the same (seed, time-bucket) pair again and you get the same value,
// which is exactly the determinism we want for a stable, repeatable animation.

// Integer hash (Bob Jenkins / lowbias32 style) — cheap, good avalanche, no libraries.
static inline uint32_t hashUint(uint32_t x) {
    x ^= x >> 16; x *= 0x7feb352du;
    x ^= x >> 15; x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

// Hash → float in [0, 1)
static inline float hash01(uint32_t x) {
    return (float)(hashUint(x) & 0x00FFFFFFu) / (float)0x01000000u;  // 24-bit precision
}

// Smoothly-interpolated pseudo-random value at time t (in arbitrary "noise time"
// units — one full unit is roughly one keyframe-to-keyframe transition).
// `seed` decorrelates independent noise streams (e.g. one per pixel).
static float valueNoise(uint32_t seed, float t) {
    float ti = floorf(t);
    float tf = t - ti;
    uint32_t i0 = seed + (uint32_t)(int32_t)ti;
    uint32_t i1 = i0 + 1u;
    float a = hash01(i0);
    float b = hash01(i1);
    float s = tf * tf * (3.0f - 2.0f * tf);  // smoothstep easing (no sudden velocity changes)
    return a + (b - a) * s;
}

// speed (0-255) → noise update rate in Hz-ish units. Tuned by ear/eye, not physics.
static inline float flickerSpeedToRate(uint8_t speed) {
    const float FLICKER_MIN_RATE = 0.5f;
    const float FLICKER_MAX_RATE = 8.0f;
    return FLICKER_MIN_RATE + ((float)speed / 255.0f) * (FLICKER_MAX_RATE - FLICKER_MIN_RATE);
}

// Returns the Flicker contribution for pixel `i` at time `t` (seconds), in [-1, +1].
// `mode` (Synchronicity) controls how the noise is distributed across the ring:
//   UNISON  — every pixel shares the exact same noise value at the exact same
//             instant — the whole ring flickers together as one light.
//   RANDOM  — every pixel gets its own independent noise stream (own seed).
//   ORDERED — every pixel samples the *same* noise stream, but delayed by an
//             amount proportional to its position — the randomness visibly
//             "chases" around the ring instead of flickering independently.
//   LOOPING — same chase idea, but the ring is folded in half and mirrored so
//             the chase travels out from pixel 0 in both directions at once,
//             meeting (and erasing the seam) at the far side of the ring.
static float flickerSignal(int i, float t, DistributionMode mode, uint8_t speed) {
    float tScaled = t * flickerSpeedToRate(speed);
    float raw;
    switch (mode) {
        case DistributionMode::UNISON:
            raw = valueNoise(0u, tScaled);  // no per-pixel seed, no delay — one shared value for all
            break;
        case DistributionMode::RANDOM:
            raw = valueNoise((uint32_t)i * 104729u, tScaled);  // large prime → decorrelated seeds
            break;
        case DistributionMode::ORDERED: {
            float delay = (float)i / (float)LED_COUNT;
            raw = valueNoise(0u, tScaled - delay);
            break;
        }
        default: {  // LOOPING
            int mirroredIndex = (i <= LED_COUNT / 2) ? i : (LED_COUNT - i);
            float delay = (float)mirroredIndex / (float)(LED_COUNT / 2);
            raw = valueNoise(0u, tScaled - delay);
            break;
        }
    }
    return raw * 2.0f - 1.0f;  // [0,1) → [-1, +1]
}

// ── Pulse: smooth sine wave ────────────────────────────────────────────────────
// speed (0-255) → pulse frequency in Hz. Deliberately slow — this is meant to
// read as a gentle "breathing" motion, not a strobe.
static inline float pulseSpeedToHz(uint8_t speed) {
    const float PULSE_MIN_HZ = 0.05f;
    const float PULSE_MAX_HZ = 1.5f;
    return PULSE_MIN_HZ + ((float)speed / 255.0f) * (PULSE_MAX_HZ - PULSE_MIN_HZ);
}

// Returns the Pulse contribution for pixel `i` at time `t` (seconds), in [-1, +1].
// `mode` (Synchronicity II) controls how the sine wave is laid out across the ring:
//   UNISON  — every pixel shares the exact same phase — the whole ring breathes
//             together as one light, no spatial variation at all.
//   RANDOM  — each pixel gets a fixed, scattered starting phase (golden-angle
//             spacing keeps neighboring pixels from lining up), so every pixel
//             breathes with its own curve rather than in sync.
//   ORDERED — spatial phase increases evenly around the ring (one full cycle
//             per lap) and that phase keeps advancing over time — a traveling
//             wave that visibly chases around the ring, exactly like Ordered
//             Flicker but smooth instead of noisy.
//   LOOPING — the sine's spatial *shape* is fixed to exactly one cycle stamped
//             around the ring (matching the static Color/Sparkle "Looping"
//             spread pattern) and does not travel; instead, the whole shape's
//             amplitude breathes in place as a synchronized envelope. This is
//             the "curve squashed to fit entirely in the number of LEDs" case.
static float pulseSignal(int i, float t, DistributionMode mode, uint8_t speed) {
    float freq = pulseSpeedToHz(speed);
    float temporalPhase = TWO_PI * freq * t;
    switch (mode) {
        case DistributionMode::UNISON:
            return sinf(temporalPhase);  // no spatial phase offset — every pixel breathes in perfect sync
        case DistributionMode::RANDOM: {
            const float GOLDEN_ANGLE = 2.399963f;  // radians (~137.5°) — scatters phases evenly
            float spatialPhase = (float)i * GOLDEN_ANGLE;
            return sinf(temporalPhase + spatialPhase);
        }
        case DistributionMode::ORDERED: {
            float spatialPhase = TWO_PI * (float)i / (float)LED_COUNT;
            return sinf(temporalPhase + spatialPhase);  // traveling wave
        }
        default: {  // LOOPING
            float spatialPhase = TWO_PI * (float)i / (float)LED_COUNT;
            return sinf(spatialPhase) * sinf(temporalPhase);  // fixed shape × breathing envelope
        }
    }
}

// ── Public API ─────────────────────────────────────────────────────────────────


void initLEDs() {
    queue_init(&_commandQueue, sizeof(uint8_t), 8);

    uint offset = pio_add_program(_pio, &ws2812_program);
    _sm = (uint)pio_claim_unused_sm(_pio, true);
    ws2812_sm_init(_pio, _sm, offset, LED_PIN);

    _ledsEnabled = true;
    _currentConfig = getDefaultConfig();  // Start with Warm Flame preset
    _configUpdated = true;
}

void setLedsEnabled(bool enabled) {
    uint8_t cmd = enabled ? 1u : 0u;
    queue_try_add(&_commandQueue, &cmd);
}

void setEffect(const char* /*name*/, uint32_t /*color*/, float /*speed*/, unsigned long /*duration*/) {
    // Effect system placeholder — rainbow is the active effect for now
}

void setAnimationConfig(const BlossomConfig& config) {
    _currentConfig = config;
    _configUpdated = true;
}

void updateLEDs() {
    static uint32_t lastFrameMs = 0;
    static bool     wasEnabled  = true;  // matches _ledsEnabled initial value

    // Drain any pending enable/disable commands from Core 0
    uint8_t cmd;
    while (queue_try_remove(&_commandQueue, &cmd)) {
        _ledsEnabled = (cmd != 0u);
    }

    if (!_ledsEnabled) {
        if (wasEnabled) {       // ON → OFF transition: blank the ring
            send_off_frame();
            wasEnabled = false;
        }
        return;
    }
    wasEnabled = true;

    // Render at ~60 FPS
    uint32_t now = millis();
    if (now - lastFrameMs < 17u) return;
    lastFrameMs = now;

    // Running "virtual rotation" accumulator for the Spin effect (Step 4), in
    // pixel units. Persists across calls so Spin keeps moving smoothly instead
    // of resetting every frame.
    static float spinAccum = 0.0f;

    // Animation clock, in seconds since boot. NOTE: millis() loses sub-millisecond
    // precision once cast to float after ~4.6 hours of continuous uptime (a 32-bit
    // float only has 24 bits of mantissa). For a slow ambient sine/noise animation
    // that shows up as imperceptible long-term phase drift, not visible stutter —
    // an acceptable trade-off for the simplicity of calling sinf()/floorf() directly
    // instead of maintaining a fixed-point animation clock.
    float tSec = (float)now / 1000.0f;
    const float dt = 1.0f / 60.0f;  // Nominal frame time, matches the ~60 FPS gate above

    // ── Animation Pipeline ─────────────────────────────────────────────────────
    // Step 1: Base color and sparkle values from _currentConfig (static spread)
    // Step 2: Flicker — noise track, moves a pixel between the two spread extremes
    // Step 3: Pulse   — sine track, same idea but smooth instead of noisy
    // Step 4: Spin    — rotates the finished pattern around the ring
    
    const ColorSettings&    color   = _currentConfig.color;
    const SparkleSettings&  sparkle = _currentConfig.sparkles;
    const AnimationTrack&   flicker = _currentConfig.flicker;
    const AnimationTrack&   pulse   = _currentConfig.pulse;
    const SpinSettings&     spin    = _currentConfig.spin;
    
    // Pre-computed sine wave values (calculated once, used forever) — these
    // drive the *static* spread distribution (Step 1) when no animation track
    // is touching a given channel.
    // Half-cycle (0° to 180°) for ORDERED/RANDOM modes: 0 → 1 → 0
    static const float halfCycleSine[LED_COUNT] = {
        0.0000f, 0.1951f, 0.3827f, 0.5556f, 0.7071f, 0.8315f, 0.9239f, 0.9808f,
        1.0000f, 0.9808f, 0.9239f, 0.8315f, 0.7071f, 0.5556f, 0.3827f, 0.1951f
    };
    // Full-cycle (0° to 360°) for LOOPING mode: 0 → 1 → 0 → -1 → 0
    static const float fullCycleSine[LED_COUNT] = {
        0.0000f,  0.3827f,  0.7071f,  0.9239f,  1.0000f,  0.9239f,  0.7071f,  0.3827f,
        0.0000f, -0.3827f, -0.7071f, -0.9239f, -1.0000f, -0.9239f, -0.7071f, -0.3827f
    };
    // Interleaving pattern for RANDOM mode (bit-reversal): which ordered position to pull from
    static const uint8_t interleavedIndices[LED_COUNT] = {
        0, 8, 4, 12, 2, 10, 6, 14, 1, 9, 5, 13, 3, 11, 7, 15
    };

    // Does *any* animation track currently claim a channel? If not, that channel
    // falls back to the static spread distribution above, exactly as before.
    // If so, the static distribution is set aside entirely — Flicker/Pulse decide
    // where each pixel sits along the spread instead (see the big comment block
    // on flickerSignal()/pulseSignal() above for why).
    bool colorAnimated   = flicker.apply_to_color    || pulse.apply_to_color;
    bool sparkleAnimated = flicker.apply_to_sparkles || pulse.apply_to_sparkles;

    // ── Pass 1: compute the whole frame into temporary arrays ──────────────────
    // We build the complete pattern here before sending anything to the PIO,
    // because Spin (Step 4, below) needs to read the finished pattern back at a
    // *shifted* index — it rotates the whole thing, so it has to exist in memory
    // first rather than being streamed straight out pixel-by-pixel.
    uint16_t hueArr[LED_COUNT];
    uint8_t  valArr[LED_COUNT];
    uint8_t  whiteArr[LED_COUNT];

    for (int i = 0; i < LED_COUNT; i++) {
        // ── Color Channel (HSV) ────────────────────────────────────────────────
        uint16_t hue = 0;
        uint8_t  val = 0;

        // Early-exit if color brightness is zero (prevents spread/animation from lighting LEDs)
        if (color.brightness != 0) {
            float offset;  // hue offset, in the same 0-255 "byte scale" as color.primary/spread

            if (!colorAnimated) {
                // Step 1: static spread distribution (Container 2's radio buttons) —
                // unchanged from the original implementation, plus the new Unison
                // option which simply pins every pixel to the primary hue (no spread).
                if (color.mode == DistributionMode::UNISON) {
                    offset = 0.0f;
                } else {
                    float sineValue;
                    if (color.mode == DistributionMode::RANDOM) {
                        sineValue = halfCycleSine[interleavedIndices[i]];
                    } else if (color.mode == DistributionMode::ORDERED) {
                        sineValue = halfCycleSine[i];
                    } else {  // LOOPING
                        sineValue = fullCycleSine[i];
                    }
                    if (color.mode == DistributionMode::LOOPING) {
                        offset = sineValue * (float)color.spread * 0.5f;
                    } else {
                        offset = (sineValue - 0.5f) * (float)color.spread;
                    }
                }
            } else {
                // Steps 2 & 3: Flicker and/or Pulse drive this pixel's position
                // along the spread instead. "position" is normalized to [-1, +1]:
                // 0 sits exactly on the primary hue, ±1 sits at one extreme of the
                // spread. Amplitude scales how far the animation is allowed to
                // swing; summing both tracks lets Flicker and Pulse layer together
                // on the same channel if the user enables both.
                float position = 0.0f;
                if (flicker.apply_to_color) {
                    position += flickerSignal(i, tSec, flicker.mode, flicker.speed) * ((float)flicker.amplitude / 255.0f);
                }
                if (pulse.apply_to_color) {
                    position += pulseSignal(i, tSec, pulse.mode, pulse.speed) * ((float)pulse.amplitude / 255.0f);
                }
                position = constrain(position, -1.0f, 1.0f);  // "capped at the spread"
                offset = position * (float)color.spread * 0.5f;
            }

            hue = (uint16_t)(((int16_t)color.primary + (int16_t)offset + 256) % 256) * 360 / 256;
            val = (uint8_t)((float)color.brightness * LED_BRIGHTNESS);
        }
        
        // ── Sparkle Channel (White) ────────────────────────────────────────────
        uint8_t white = 0;

        // Early-exit if sparkle brightness is zero (prevents spread/animation from lighting LEDs)
        if (sparkle.brightness != 0) {
            float offset;

            if (!sparkleAnimated) {
                if (sparkle.mode == DistributionMode::UNISON) {
                    offset = 0.0f;
                } else {
                    float sineValue;
                    if (sparkle.mode == DistributionMode::RANDOM) {
                        sineValue = halfCycleSine[interleavedIndices[i]];
                    } else if (sparkle.mode == DistributionMode::ORDERED) {
                        sineValue = halfCycleSine[i];
                    } else {  // LOOPING
                        sineValue = fullCycleSine[i];
                    }
                    if (sparkle.mode == DistributionMode::LOOPING) {
                        offset = sineValue * (float)sparkle.spread * 0.5f;
                    } else {
                        offset = (sineValue - 0.5f) * (float)sparkle.spread;
                    }
                }
            } else {
                float position = 0.0f;
                if (flicker.apply_to_sparkles) {
                    position += flickerSignal(i, tSec, flicker.mode, flicker.speed) * ((float)flicker.amplitude / 255.0f);
                }
                if (pulse.apply_to_sparkles) {
                    position += pulseSignal(i, tSec, pulse.mode, pulse.speed) * ((float)pulse.amplitude / 255.0f);
                }
                position = constrain(position, -1.0f, 1.0f);
                offset = position * (float)sparkle.spread * 0.5f;
            }

            white = (uint8_t)constrain((int16_t)sparkle.brightness + (int16_t)offset, 0, 255);
            // Apply LED_BRIGHTNESS cap to white channel
            white = (uint8_t)((float)white * LED_BRIGHTNESS);
        }

        hueArr[i]   = hue;
        valArr[i]   = val;
        whiteArr[i] = white;
    }

    // ── Step 4: Spin ───────────────────────────────────────────────────────────
    // Spin runs *after* the color/flicker/pulse pattern above is fully computed —
    // it never changes what color a pixel would show, it just rotates the whole
    // finished pattern around the ring by shifting which array index feeds which
    // physical LED. speed (-128..127) maps to roughly ±half a rotation per second
    // at the extremes; that range was chosen for a hypnotic-but-legible spin on a
    // 16-pixel ring. (A literal "N pixels per PIO frame" reading of speed would
    // spin the ring many times *per frame* at high settings — just a strobe.)
    const float MAX_SPIN_PIXELS_PER_SEC = 8.0f;
    float spinPixelsPerSec = ((float)spin.speed / 128.0f) * MAX_SPIN_PIXELS_PER_SEC;
    spinAccum += spinPixelsPerSec * dt;
    // Keep the accumulator wrapped into [0, LED_COUNT) so it never loses float
    // precision over hours of continuous runtime.
    spinAccum = fmodf(spinAccum, (float)LED_COUNT);
    if (spinAccum < 0.0f) spinAccum += (float)LED_COUNT;
    int shift = (int)spinAccum;  // whole-pixel shift — 16 LEDs is too coarse a ring to bother interpolating fractional shifts

    // ── Pass 2: apply Spin (independently per channel) and stream to the PIO ───
    for (int i = 0; i < LED_COUNT; i++) {
        int colorIndex   = spin.apply_to_color    ? ((i - shift) % LED_COUNT + LED_COUNT) % LED_COUNT : i;
        int sparkleIndex = spin.apply_to_sparkles ? ((i - shift) % LED_COUNT + LED_COUNT) % LED_COUNT : i;

        uint16_t hue   = hueArr[colorIndex];
        uint8_t  val   = valArr[colorIndex];
        uint8_t  white = whiteArr[sparkleIndex];

        // ── Pack and Send ──────────────────────────────────────────────────────
        // Convert HSV to RGB
        uint8_t sat    = 255;  // Full saturation for vivid colors
        uint8_t region = hue / 60u;
        uint8_t rem    = (uint8_t)((hue % 60u) * 255u / 60u);
        uint8_t p = (uint8_t)((uint32_t)val * (255u - sat) / 255u);
        uint8_t q = (uint8_t)((uint32_t)val * (255u - (uint32_t)sat * rem  / 255u) / 255u);
        uint8_t t = (uint8_t)((uint32_t)val * (255u - (uint32_t)sat * (255u - rem) / 255u) / 255u);
        uint8_t r, g, b;
        switch (region) {
            case 0:  r = val; g = t;   b = p;   break;
            case 1:  r = q;   g = val; b = p;   break;
            case 2:  r = p;   g = val; b = t;   break;
            case 3:  r = p;   g = q;   b = val; break;
            case 4:  r = t;   g = p;   b = val; break;
            default: r = val; g = p;   b = q;   break;
        }
        
        // Pack SK6812 GRBW word (G, R, B, W in MSB-first order)
        uint32_t pixel = ((uint32_t)g << 24) | ((uint32_t)r << 16) | ((uint32_t)b << 8) | white;
        put_pixel(pixel);
    }
    
    send_reset();
}
