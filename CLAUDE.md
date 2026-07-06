# Blossom Project - Development Guidelines

## Project Overview
Raspberry Pi Pico 2W networked lighting display with WiFi provisioning portal.

## Build & Test Requirements

### Always Test Builds
**IMPORTANT**: Before marking any code changes complete, **ALWAYS run a build** to check for compilation errors.

**Build Command:**
```bash
cmd //c "C:\Users\fargo\.platformio\penv\Scripts\platformio.exe run 2>&1"
```

- If build fails, fix errors before proceeding
- Check for syntax errors, type mismatches, and missing includes
- Only report work as complete after successful compilation

### Upload to Device
```bash
cmd //c "C:\Users\fargo\.platformio\penv\Scripts\platformio.exe run --target upload"
```

### Serial Monitor
```bash
cmd //c "C:\Users\fargo\.platformio\penv\Scripts\platformio.exe device monitor"
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

## Development Workflow
1. Make code changes
2. **Run build test** (see command above)
3. Fix any compilation errors
4. Upload to device
5. Test via serial monitor and web interface
6. Only then mark task complete
