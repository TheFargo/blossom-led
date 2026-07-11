// ###########################################################################
//                                  BLOSSOM
//                         Programmable Light Display
//                         A Dave "Fargo" Kosak Joint
//                                July,  2026
//       AI Models Consulted: Gemini 3.5 Thinking, Claude Sonnet 4.5-4.6
// ###########################################################################

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <LittleFS.h>
#include <LEAmDNS.h>

#include "credentials.h"
#include "wifi_manager.h"
#include "web_handlers.h"
#include "led_controller.h"

// LED pin (Pico W uses WiFi chip LED)
#ifndef LED_BUILTIN
#define LED_BUILTIN 25
#endif

// Global server instances (referenced via extern in web_handlers.h)
WebServer server(80);
DNSServer dnsServer;

// LED state tracking (exposed to web_handlers.cpp via extern)
bool ledState = true;

// System state
enum SystemMode {
  MODE_PROVISIONING,  // AP mode: no credentials, auth failure, or network unavailable
  MODE_CONNECTED      // STA mode: fully on WiFi, WebServer running
};
SystemMode currentMode = MODE_PROVISIONING;

// Deferred reboot - lets HTTP response flush before AP goes down
bool rebootPending = false;
unsigned long rebootAt = 0;

void scheduleReboot(unsigned long delayMs) {
  rebootPending = true;
  rebootAt = millis() + delayMs;
}

// Factory reset button state
unsigned long bootselPressStart = 0;
bool bootselPressedLastLoop = false;
const unsigned long FACTORY_RESET_HOLD_TIME = 5000;  // 5 seconds

// --- Core 0: Setup & Loop ---

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n=================================");
  Serial.println("Blossom - Programmable Light Display");
  Serial.println("=================================\n");

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  if (!LittleFS.begin()) {
    Serial.println("! LittleFS mount failed -- run: pio run --target uploadfs");
  } else {
    Serial.println("+ LittleFS mounted");
  }

  String savedSSID, savedPassword;
  if (loadCredentials(savedSSID, savedPassword)) {
    Serial.println("Found saved credentials, attempting connection...");

    if (connectToWiFi(savedSSID, savedPassword, 20)) {
      currentMode = MODE_CONNECTED;
      digitalWrite(LED_BUILTIN, HIGH);

      if (MDNS.begin("blossom")) {
        Serial.println("+ mDNS responder started -- hostname: blossom.local");
        MDNS.addService("http", "tcp", 80);
      } else {
        Serial.println("x mDNS setup failed");
      }

      setupConnectedRoutes();

      Serial.println("\n=================================");
      Serial.println("+ CONNECTED TO WIFI");
      Serial.print("Network: ");
      Serial.println(WiFi.SSID());
      Serial.print("IP: ");
      Serial.println(WiFi.localIP());
      Serial.println("Hostname: blossom.local");
      Serial.println("=================================\n");
      return;
    }

    // Distinguish why the connection failed before deciding what to do with credentials.
    // WL_CONNECT_FAILED means the password was rejected — credentials are definitively wrong.
    // Any other failure (timeout, WL_NO_SSID_AVAIL) means the network was unreachable —
    // could be temporary outage OR a new location.  Keep credentials either way: a power
    // cycle will retry automatically, and the provisioning AP lets the user reconfigure
    // immediately without needing to know about stored credentials.
    if (WiFi.status() == WL_CONNECT_FAILED) {
      Serial.println("x Authentication failed -- clearing credentials");
      clearCredentials();
    } else {
      Serial.println("x Network unreachable -- credentials preserved, starting provisioning AP");
      Serial.println("  (Power cycle will retry stored credentials automatically)");
    }
    // fall through to start the provisioning AP in both cases
  } else {
    Serial.println("No saved credentials found");
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
