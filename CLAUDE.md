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
- **CRITICAL**: Provisioning mode must use `WIFI_AP_STA` mode (not `WIFI_AP`)
  - This allows testing WiFi credentials while keeping the AP alive
  - Without this, switching to STA mode disconnects the client before HTTP response is sent
  - Client sees "Connection failed" even though connection succeeds and device reboots

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
- `POST /api/connect` - Accepts `{ssid, password}`, attempts connection, reboots on success

**Connected Mode:**
- `GET /` - Serves "Hello World" status page
- `GET /api/status` - Returns JSON with `{status, ssid, ip, rssi}`

### Reboot Behavior
- After successful WiFi connection, device reboots using `rp2040.reboot()`
- This ensures clean state transition from AP mode to Station mode
- 1 second delay before reboot allows HTTP response to be sent

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
