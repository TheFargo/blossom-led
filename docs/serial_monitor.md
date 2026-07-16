# Using the Serial Monitor: Blossom Programmable Light Display

Since your little Pico 2W doesn't have a screen (_yet_...) a common way to debug code in action is by running a serial monitor. Think of it as a little telephone line that your Pico can use to type out text messages to you. 

This guide will show you how to turn on and use the Serial monitor to listen in to what your Blossom is up to! 

---

## Table of Contents

1. [Activating the Serial Monitor in Code](#1-activating-the-serial-monitor-in-code)
2. [Listening to the Serial Monitor](#2-listening-to-the-serial-monitor)

---

## 1. Activating the Serial Monitor in Code

By default, the Serial Monitor is off so that nothing is slowing down your Blossom. If you want to turn it on, just look for this line near the top of `/src/main.cpp`:

    bool serialDebug = false; 

Change that value to "true," compile and upload the build, and your debugger will run!

### Printing to the Serial Monitor

Printing to the serial monitor is as easy as this:

    Serial.println("+ CONNECTED TO WIFI"); 

Of course, that line will error out if the serial monitor is inactive. So you should only write to the monitor if serialDebug = true, like so:

    if (serialDebug) {
        Serial.println("\n=================================");
        Serial.println("Blossom - Programmable Light Display");
        Serial.println("=================================\n");
    }

## 2. Listening to the Serial Monitor

PlatformIO will automatically print the contents of the Serial Monitor after a successful upload: Keep your eyes open for the Blossom welcome message pasted above. Don't see it? You might have to manually open up a serial terminal: press the button with the "plug" icon in your Platform IO toolbar at the bottom of your VS Code window (conveniently labelled "PlatformIO: Serial Monitor").

When the monitor is working, you'll see status updates as your Blossom searches Wifi Networks, gets and saves credentials, connects to your LAN, etc.

This is a great debugging tool for seeing what your deivce is up to!