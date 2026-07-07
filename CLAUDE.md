# Blossom Project - Development Guidelines

## Project Overview
Raspberry Pi Pico 2W networked lighting display with WiFi provisioning portal.

## Build & Test Requirements

### Always Test Builds
**IMPORTANT**: Before marking any code changes complete, **ALWAYS run a build** to check for compilation errors.

**Build Command:**
```powershell
C:\Users\fargo\.platformio\penv\Scripts\platformio.exe run
```

- If build fails, fix errors before proceeding
- Check for syntax errors, type mismatches, and missing includes
- Only report work as complete after successful compilation

### Upload to Device
```powershell
C:\Users\fargo\.platformio\penv\Scripts\platformio.exe run --target upload
```

### Upload Filesystem
```powershell
C:\Users\fargo\.platformio\penv\Scripts\platformio.exe run --target uploadfs
```

### Serial Monitor
```powershell
C:\Users\fargo\.platformio\penv\Scripts\platformio.exe device monitor
```

## Platform Details
- **Board**: Raspberry Pi Pico 2W (RP2350)
- **Framework**: Arduino (Earle Philhower Core)
- **Platform**: https://github.com/maxgerhardt/platform-raspberrypi.git

## Known Issues & Fixes

### String Concatenation
❌ **Don't do this:**
```cpp
json += "text" + variableString + "more";  // Fails with const char* types
```

✅ **Do this instead:**
```cpp
json += "text";
json += variableString;
json += "more";
```

### WiFi Access Point Configuration
- IP configuration (`WiFi.softAPConfig()`) must happen **before** `WiFi.softAP()`
- Open network: use `WiFi.softAP(ssid)` with NO password parameter
- **CRITICAL**: Use pure `WIFI_AP` mode during provisioning — do NOT use `WIFI_AP_STA`
  - The CYW43439 is a single-radio chip; AP+STA mode causes AP dropouts during authentication
  - This means you cannot reliably test credentials while the AP is running
  - Any attempt to call `WiFi.begin()` in AP mode will disrupt existing client connections

### Provisioning Pattern (Industry Standard)
The correct pattern for single-radio chips — do not try to be clever with AP+STA:
1. Device in pure `WIFI_AP` mode — fully stable, no radio contention
2. User submits credentials via captive portal
3. Device **saves credentials to flash** and sends HTTP 200 response
4. Device **defers reboot** (~1.5s) via a flag checked in `loop()` — never call `rp2040.reboot()` directly inside a handler, as it kills the TCP stack before the response flushes
5. Device reboots into pure `WIFI_STA` mode and tests credentials
6. On failure: clears credentials, falls back to provisioning AP mode
7. On success: starts mDNS, serves connected page

### WiFi Status Constants
- `WL_WRONG_PASSWORD` is not available in the Earle Philhower core
- Use `WL_CONNECT_FAILED` for general connection failures
- Use `WL_NO_SSID_AVAIL` to detect network not found

## Implementation Notes

### Credential Storage
- WiFi credentials are stored in LittleFS at `/wifi_creds.txt`
- Format: SSID on first line, password on second line
- LittleFS is safer and easier than raw flash APIs
- Passwords are masked with asterisks in serial output for security

### State Machine
The firmware operates in two modes:
- **MODE_PROVISIONING**: Access Point mode with captive portal (192.168.4.1)
- **MODE_CONNECTED**: Connected to WiFi with mDNS at blossom.local

On boot, the system:
1. Checks for stored credentials in LittleFS
2. If found, attempts to connect (20 second timeout)
3. On success: enters MODE_CONNECTED, starts mDNS, serves Hello World page
4. On failure: clears bad credentials, enters MODE_PROVISIONING

### mDNS Implementation
- Requires `#include <LEAmDNS.h>` (not ESP8266mDNS.h)
- Hostname: `blossom.local`
- Service: `_http._tcp` on port 80
- Must call `MDNS.update()` in loop() when connected

### API Endpoints

**Provisioning Mode:**
- `GET /` - Serves provisioning HTML page
- `GET /api/scan` - Returns JSON list of WiFi networks
- `POST /api/connect` - Accepts `{ssid, password}`, saves credentials to flash, schedules deferred reboot

**Connected Mode:**
- `GET /` - Serves "Hello World" status page
- `GET /api/status` - Returns JSON with `{status, ssid, ip, rssi}`

### Reboot Behavior
- **Never call `rp2040.reboot()` inside an HTTP handler** — the TCP stack shuts down before the response flushes
- Call `scheduleReboot(1500)` instead — this is a function defined in `main.cpp` that sets `rebootPending = true` and a future timestamp; `loop()` fires it after the TCP stack has had time to flush
- `scheduleReboot()` is declared in `web_handlers.h` (consumed there) and defined in `main.cpp`
- This guarantees the HTTP 200 response reaches the browser before the AP disappears

### Factory Reset
- Hold BOOTSEL button for 5 seconds at any time to trigger factory reset
- LED blinks rapidly during button hold for visual feedback
- Serial monitor shows countdown: "Factory reset in X seconds..."
- When triggered: clears WiFi credentials, flashes LED 10 times, reboots to provisioning mode
- Button release before 5 seconds cancels the reset
- Uses `BOOTSEL` constant to read button state (Earle Philhower core)

## Development Workflow

There are now **two independent upload workflows** — use only the one that matches what changed:

### C++ code changed (any .cpp / .h file)
1. Run build: `platformio.exe run`
2. Fix any errors
3. Upload firmware: `platformio.exe run --target upload`
4. Test via serial monitor

### HTML/CSS/JS changed (data/provisioning.html or data/connected.html)
1. Upload filesystem only: `platformio.exe run --target uploadfs`
2. No recompile or firmware upload needed
3. Reload the page in the browser

### Both changed
Run `uploadfs` first, then `upload`.

## Code Architecture

The firmware is split into focused modules. `main.cpp` is the state machine only — ~190 lines.

```
include/                    src/
  credentials.h    <-->       credentials.cpp     save/load/clearCredentials, CRED_FILE
  wifi_manager.h   <-->       wifi_manager.cpp    connectToWiFi(), startProvisioningAP()
  web_handlers.h   <-->       web_handlers.cpp    all HTTP handlers + setupProvisioningRoutes/setupConnectedRoutes
  led_controller.h <-->       led_controller.cpp  initLEDs(), setEffect(), updateLEDs() [STUB]
                              main.cpp            setup(), loop(), setup1(), loop1()

data/
  provisioning.html           WiFi setup page — served from LittleFS
  connected.html              Status page — served from LittleFS
  fonts/Gluten.ttf
  fonts/Fredoka.ttf
```

### Cross-file dependency pattern
`WebServer server(80)` and `DNSServer dnsServer` are defined as globals in `main.cpp`. Modules that need them use `extern` declarations in their header:
```cpp
// web_handlers.h
extern WebServer server;
```
All HTTP handler functions in `web_handlers.cpp` are `static` (file-local) — only the two route-setup functions are public API.

### HTML pages are served from LittleFS
Both pages live in `data/` and are streamed with `server.streamFile()`. The PROGMEM approach has been retired. To update a page: edit the HTML file, run `uploadfs`. No recompile.

### scheduleReboot()
Defined in `main.cpp`, declared in `web_handlers.h`. Handlers call this instead of touching raw globals:
```cpp
scheduleReboot(1500);  // 1.5 second delay before rp2040.reboot() fires in loop()
```

## LED Implementation

The LED controller stub is ready in `include/led_controller.h` and `src/led_controller.cpp`.

### Dual-core entry points (Earle Philhower Arduino)
`setup1()` and `loop1()` are defined at the bottom of `main.cpp`. Core 1 starts automatically after Core 0's `setup()` completes. When LED work begins:
1. Uncomment `initLEDs()` in `setup1()`
2. Uncomment `updateLEDs()` in `loop1()`

### LED controller interface
```cpp
void initLEDs();
void setEffect(const char* name, uint32_t color, float speed, unsigned long duration);
void updateLEDs();  // called from loop1() at ~60 FPS
```
- `color` is a packed `uint32_t` (e.g. GRBW format for NeoPixels)
- `duration` in milliseconds, 0 = loop until next command
- `speed` is a float modifier (1.0 = normal rate)

### Hardware notes for LED wiring
- NeoPixel ring powered from **VBUS (5V)** — not 3.3V
- Pico GPIO is 3.3V; NeoPixels need ≥3.5V signal — a **logic level shifter** (74AHCT125 or equivalent) is **required** between GPIO and LED data line
- LED data GPIO pin: TBD — pick a free GPIO and define `LED_PIN` in `led_controller.cpp`
- LED count: TBD — define `LED_COUNT` in `led_controller.cpp`

### Inter-core communication
Effect commands arrive on Core 0 (HTTP handler) and must reach Core 1 (animation engine). Use the Pico SDK hardware FIFO queue — **do not use a plain global struct without a mutex**:
```cpp
#include <pico/util/queue.h>
queue_t effectQueue;  // initialized in initLEDs(), push from setEffect(), pop in updateLEDs()
```

### NeoPixel library decision
Library not yet chosen. Options:
- **Adafruit_NeoPixel** — simple, well-documented, single-file
- **FastLED** — more animation primitives, wider community
- **PIO direct** — maximum control and performance, but requires writing PIO assembly
