# Raspberry Pi Pico 2W - PlatformIO Project

## Setup Instructions

### 1. Install PlatformIO Extension
If not already installed:
- Open VS Code Extensions (Ctrl+Shift+X)
- Search for "PlatformIO IDE"
- Install it and reload VS Code

### 2. First Build
- Open this folder in VS Code
- PlatformIO will detect `platformio.ini` and initialize the project
- Click the PlatformIO icon in the left sidebar
- Under "PROJECT TASKS" → "pico2w" → "General" → click "Build"

### 3. Upload to Pico 2W

**Method 1: Automatic Upload (Recommended)**
1. Connect Pico 2W via USB
2. Click the Upload button (→) in the PlatformIO toolbar at the bottom
3. PlatformIO will automatically handle reboot and flash

**Method 2: Manual Bootloader Mode**
If automatic upload fails:
1. Hold BOOTSEL button on Pico 2W
2. Connect USB (or press reset while holding BOOTSEL)
3. Release BOOTSEL when drive appears
4. Click Upload in PlatformIO

### 4. Monitor Serial Output
- Click the Serial Monitor plug icon in the bottom toolbar
- Set baud rate to 115200
- You should see "LED ON" and "LED OFF" messages

## Configuration Details

- **Board**: Raspberry Pi Pico 2W (RP2350)
- **Framework**: Arduino (Earle Philhower's core)
- **Platform**: Custom RP2350-enabled platform
- **LED Pin**: GPIO 25 (built-in)

## Troubleshooting

If upload fails with "No device found":
- Install picotool: `pip install picotool`
- Or use bootloader mode (Method 2 above)

If LED doesn't blink:
- Check Serial Monitor for debug messages
- Verify USB connection provides power
- Try pressing the reset button
