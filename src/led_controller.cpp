#include "led_controller.h"
#include "animation_config.h"
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
    static uint16_t phase       = 0;
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

    // TODO: Replace rainbow with full BlossomConfig animation pipeline:
    //   1. Apply base color/sparkle settings from _currentConfig
    //   2. Layer flicker (noise) animation if enabled
    //   3. Layer pulse (sine) animation if enabled
    //   4. Apply spin (pixel rotation) if enabled
    //   For now, rainbow serves as placeholder
    send_rainbow_frame(phase);
    phase = (uint16_t)((phase + 2u) % 360u);
}
