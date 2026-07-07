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
- Use a deferred reboot instead: set `rebootPending = true` and `rebootAt = millis() + 1500` in the handler, check in `loop()`
- This guarantees the HTTP 200 response reaches the browser before the AP disappears

### Factory Reset
- Hold BOOTSEL button for 5 seconds at any time to trigger factory reset
- LED blinks rapidly during button hold for visual feedback
- Serial monitor shows countdown: "Factory reset in X seconds..."
- When triggered: clears WiFi credentials, flashes LED 10 times, reboots to provisioning mode
- Button release before 5 seconds cancels the reset
- Uses `BOOTSEL` constant to read button state (Earle Philhower core)

## Development Workflow
1. Make code changes
2. **Run build test** (see command above)
3. Fix any compilation errors
4. Upload to device
5. Test via serial monitor and web interface
6. Only then mark task complete
