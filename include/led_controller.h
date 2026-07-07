#pragma once
#include <Arduino.h>

// LED Controller — NeoPixel animation engine
// Runs on Core 1 via setup1() / loop1() (Earle Philhower dual-core)
// TODO: Choose NeoPixel library (Adafruit_NeoPixel, FastLED, or PIO direct)

void initLEDs();
void setEffect(const char* name, uint32_t color, float speed, unsigned long duration);
void updateLEDs();
