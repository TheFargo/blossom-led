#pragma once
#include <Arduino.h>

// LED Controller — SK6812 RGBW NeoPixel animation engine (PIO direct-drive)
// Runs on Core 1 via setup1() / loop1() (Earle Philhower dual-core)

void initLEDs();
void setLedsEnabled(bool enabled);
void setEffect(const char* name, uint32_t color, float speed, unsigned long duration);
void updateLEDs();
