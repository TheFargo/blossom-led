// ###########################################################################
//                                  BLOSSOM
//                  WiFi-Enabled Programmable Light Display
//                         A Dave "Fargo" Kosak Joint
//                                July, 2026
//       AI Models Consulted: Gemini 3.5 Thinking, Claude Sonnet 4.5-4.6
// ###########################################################################
//                  Copyright 2026 Word Smith & Wright, LLC
//  Project: Blossom - WiFi-Enabled Programmable Light Display
//  Description: Firmware for Raspberry Pi Pico 2W local wifi LED controller
//  Author: Dave Kosak <dave@wordsmithandwright.com>
//
//   Licensed under the Apache License, Version 2.0 (the "License");
//   you may not use this file except in compliance with the License.
//   You may obtain a copy of the License at
//
//       http://www.apache.org/licenses/LICENSE-2.0
//
//   Unless required by applicable law or agreed to in writing, software
//   distributed under the License is distributed on an "AS IS" BASIS,
//   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//   See the License for the specific language governing permissions and
//   limitations under the License.
// ###########################################################################
//   HARDWARE SETUP
//   - Raspberry Pi Pico 2W (RP2350) microcontroller
//   - An array of SK6812 LEDs:
//     - Tested with Adafruit Neopixel 16-pixel RGBW ring 
//     - Powered via 5V USB - use separate power supply for larger LED arrays
//   - Connections: 
//     - Powered Micro-USB cable to Raspberry Pi Pico 2W
//     - LED data line to GP16 (Pin 21 on the Pico - far corner from USB port)
//     - LED power to VBUS (Pin 40 on the Pico) or external 5V supply
//     - LED ground to Pico GND (Pin 38, 33, 28, or 23 on the Pico)
//   NOTES ABOUT SK6812 LEDs:
//   - SK6812 expects 3.5V from the data line, but our Pico only outputs 3.3V.
//     Seems to work reliably on Adafruit's Neopixel rings. 
//     Incorporate a logic-level shifter if you have issues with your specific LED array.
//   - These lights can be BRIGHT! Use the LED_BRIGHTNESS constant in led_controller.cpp
//     to cap the maximum brightness. If you'd like to run the lights at full intensity,
//     it's definitely recommended to give them their own 5V power supply.
// ###########################################################################
//   SYSTEM ARCHITECTURE
//                  ┌───────────────────────────────────┐
//                  │            RP 2350 CPU            │
//       ┌────────┐ │ ┌──────────────┐ ┌──────────────┐ │
//       │  File  ├─┴─┤    CORE 0:   ├─┤    CORE 1:   │ │
//       │ System ├─┬─┤ Wifi and Web ├─┤LED Controller│ │
//       └────────┘ │ └──────┬───────┘ └───────┬──────┘ │
//                  └────────┼─────────────────┼────────┘
//                      ┌────┴─────┐     ┌─────┴────┐   
//                      │Wifi Chip │     │PIO State │   
//                      │          │     │ Machine  │   
//                      └──────────┘     └─────┬────┘   
//                       ││▲    ▼││         *  │  *     
//                     Local Network      *  LEDs   *   
//                                          *  *  *     
// The Pico could probably manage everything on a single core, but let's show off!
// We use the dual-core to manage connections while ensuring the LED animation is unbroken.
//   Core 0: WiFi, WebServer, and provisioning.
//   Core 1: Calculating LED animations and sending updates to them via PIO.
// The Pico comes with built-in "Programmable I/O" ("PIO") state machines.
//   These are little tiny machine-code processors with perfect timing.
//   SK6812 LEDs expect signals to come in at very specific intervals;
//   Offloading this timing to a PIO state machine frees up the main CPU. Very fancy.
//   See the led_controller.cpp file for all the details and machine code!
// ###########################################################################
//
// ###########################################################################
// ##                        MAIN.cpp - Entry Point                         ##
// ###########################################################################
//    Initializes and starts the loops for both cores.
//    Core 0 manages the main device state:
//      - MODE_PROVISIONING: Not connected to wifi. Posts the provisioning server.
//      - MODE_CONNECTED: Connected to wifi. Runs webserver and sends LED updates to core 1.
//    Hardware Reset is also handled on core 0:
//      Hold BOOTSEL button for 5 seconds in any mode to clear credentials and reboot.
//    Core 1 is dedicated to the LED controller and animations - see led_controller.cpp.

// Necessary libraries for core operation, web services, and file system management
#include <Arduino.h>        // Core Arduino library for Pico W
#include <WiFi.h>           // WiFi support for Pico W
#include <WebServer.h>      // HTTP server for handling web requests
#include <DNSServer.h>      // DNS server for handling captive portal when provisioning
#include <LEAmDNS.h>        // mDNS support for local network discovery ("blossom.local")
#include <LittleFS.h>       // File system for storing credentials, web pages and assets
// Project-specific headers for credentials, network requests, and LED control
#include "credentials.h"    // Load/Save WiFi credentials to LittleFS
#include "wifi_manager.h"   // Handles WiFi connection logic and provisioning
#include "web_handlers.h"   // HTTP request handlers for the web server
#include "led_controller.h" // Controls the LED animations and updates (core 1)

// On-Board LED pin (Pico W uses WiFi chip LED) Used to display Blossom's connected status
#ifndef LED_BUILTIN
#define LED_BUILTIN 25
#endif

// Global server instances (used in web_handlers.h)
WebServer server(80);
DNSServer dnsServer;

// System state
enum SystemMode {
  MODE_PROVISIONING,  // AP mode: no credentials, auth failure, or network unavailable
  MODE_CONNECTED      // STA mode: fully on WiFi, WebServer running
};
SystemMode currentMode = MODE_PROVISIONING;

// Enable Serial Output to Watch debug messages on your serial monitor!
bool serialDebug = true;  // Usually false for production builds

// LED state tracking (If the web handler needs to look at the state of the LED)
bool ledState = true;

// Deferred reboot - Allows messages to complete before restarting system
bool rebootPending = false;
unsigned long rebootAt = 0;
// Schedule a reboot after a specified delay (in milliseconds)
void scheduleReboot(unsigned long delayMs) {
  rebootPending = true;
  rebootAt = millis() + delayMs;
}

// Factory reset button state (Uses the built-in Pico 2W BOOTSEL button)
unsigned long bootselPressStart = 0;
bool bootselPressedLastLoop = false;
const unsigned long FACTORY_RESET_HOLD_TIME = 5000;  // 5 seconds

// ###########################################################################
// ##                   MAIN.cpp - Core 0 Setup and Loop                    ##
// ###########################################################################

void setup() {
  // Initialize Serial Output (if active) and give it some time to start up
  if (serialDebug) {
    Serial.begin(115200);
    delay(1000);  // Allow time for the serial monitor to connect
    Serial.println("\n=================================");
    Serial.println("Blossom - Programmable Light Display");
    Serial.println("=================================\n");
  }

  // Initialize the on-board LED pin for status indication
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Initialize the file system (LittleFS)
  // Report status to serial if enabled
  if (!LittleFS.begin()) {
    if (serialDebug) Serial.println("! LittleFS mount failed. Run: pio run --target uploadfs");
  } else {
    if (serialDebug) Serial.println("+ LittleFS mounted");
  }

  // Try to load saved WiFi credentials from the file system
  String savedSSID, savedPassword;
  int timeoutSeconds = 20;  
  if (loadCredentials(savedSSID, savedPassword)) {
    if (serialDebug) Serial.println("Found saved credentials, attempting connection...");

    if (connectToWiFi(savedSSID, savedPassword, timeoutSeconds)) {
      // Successful connection! Set mode and turn indicator LED on
      currentMode = MODE_CONNECTED;
      digitalWrite(LED_BUILTIN, HIGH);

      // Start mDNS responder for local network discovery: "blossom.local"
      if (MDNS.begin("blossom")) {
        if (serialDebug) Serial.println("+ mDNS responder started. Hostname: blossom.local");
        MDNS.addService("http", "tcp", 80);
      } else {
        if (serialDebug) Serial.println("! mDNS setup failed. Check network configuration.");
      }

      // Set up web server routes for connected mode
      setupConnectedRoutes();

      if (serialDebug) {
        Serial.println("\n=================================");
        Serial.println("+ CONNECTED TO WIFI");
        Serial.print("Network: ");
        Serial.println(WiFi.SSID());
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        Serial.println("Hostname: blossom.local");
        Serial.println("=================================\n");
      }

      // If connected, setup is finished! 
      return;
    }

    // Distinguish why the connection failed before deciding what to do with credentials.
    // WL_CONNECT_FAILED means the password was rejected — credentials are definitively wrong.
    // Any other failure (timeout, WL_NO_SSID_AVAIL) means the network was unreachable —
    // could be temporary outage OR a new location.  Keep credentials either way: a power
    // cycle will retry automatically, and the provisioning AP lets the user reconfigure
    // immediately without needing to know about stored credentials.
    if (WiFi.status() == WL_CONNECT_FAILED) {
      if (serialDebug) Serial.println("x Authentication failed -- clearing credentials");
      clearCredentials();
    } else {
      if (serialDebug) {
        Serial.println("x Network unreachable -- credentials preserved, starting provisioning AP");
        Serial.println("  (Power cycle will retry stored credentials automatically)");
      }
    }
    // fall through to start the provisioning AP in both cases
  } else {
    if (serialDebug) Serial.println("No saved credentials found");
  }

  currentMode = MODE_PROVISIONING;
  startProvisioningAP(dnsServer);
  setupProvisioningRoutes();
  digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {
  bool bootselPressed = BOOTSEL;

  if (bootselPressed && !bootselPressedLastLoop) {
    bootselPressStart = millis();
    Serial.println("\nBOOTSEL button pressed - hold for 5 seconds to factory reset...");
  }

  if (bootselPressed && bootselPressedLastLoop) {
    unsigned long holdDuration = millis() - bootselPressStart;
    digitalWrite(LED_BUILTIN, (millis() / 100) % 2);

    static unsigned long lastCountdown = 0;
    if (millis() - lastCountdown > 1000) {
      int secondsLeft = (FACTORY_RESET_HOLD_TIME - holdDuration) / 1000;
      if (secondsLeft >= 0) {
        Serial.print("Factory reset in ");
        Serial.print(secondsLeft + 1);
        Serial.println(" seconds...");
      }
      lastCountdown = millis();
    }

    if (holdDuration >= FACTORY_RESET_HOLD_TIME) {
      Serial.println("\n=================================");
      Serial.println("FACTORY RESET TRIGGERED");
      Serial.println("=================================");

      for (int i = 0; i < 10; i++) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(50);
        digitalWrite(LED_BUILTIN, LOW);
        delay(50);
      }

      clearCredentials();
      Serial.println("Rebooting to provisioning mode...");
      delay(500);
      rp2040.reboot();
    }
  }

  if (!bootselPressed && bootselPressedLastLoop) {
    unsigned long holdDuration = millis() - bootselPressStart;
    if (holdDuration < FACTORY_RESET_HOLD_TIME) {
      Serial.println("BOOTSEL button released - factory reset cancelled");
    }
  }

  bootselPressedLastLoop = bootselPressed;

  if (rebootPending && millis() >= rebootAt) {
    rp2040.reboot();
  }

  if (currentMode == MODE_PROVISIONING) {
    dnsServer.processNextRequest();

    if (!bootselPressed) {
      static unsigned long lastBlink = 0;
      if (millis() - lastBlink > 2000) {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
        lastBlink = millis();
      }
    }
  } else {  // MODE_CONNECTED
    MDNS.update();

    // WiFi watchdog: if the connection drops (e.g. router reboot, brief outage),
    // the WebServer and lwIP stack won't recover on their own.  After a 30-second
    // grace period we reboot — the device reconnects in ~10 s and re-advertises
    // blossom.local via mDNS.  Core 1 / LEDs are unaffected during the wait.
    static bool          wifiWasLost = false;
    static unsigned long wifiLostAt  = 0;
    if (WiFi.status() != WL_CONNECTED) {
      if (!wifiWasLost) {
        wifiWasLost = true;
        wifiLostAt  = millis();
        Serial.println("! WiFi disconnected -- will reboot in 30 s if not restored");
      } else if (millis() - wifiLostAt >= 30000UL) {
        Serial.println("! WiFi lost for 30 s -- rebooting to reconnect");
        delay(100);
        rp2040.reboot();
      }
    } else {
      wifiWasLost = false;
    }

    if (!bootselPressed) {
      digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
    }
  }

  server.handleClient();
}

// --- Core 1: LED Animation Engine ---

void setup1() {
  // Core 1 starts here — LED controller initialization
  initLEDs();
}

void loop1() {
  // Core 1 main loop — runs animation engine at ~30 FPS independently of Core 0
  updateLEDs();
}
