# Installation Guide: Blossom Programmable Light Display

This guide walks you through setting up the development environment for **Blossom**.
We'll be using two tools familiar to embedded systems designers: 

  - Visual Studio Code: A software development environment.
  - Platform IO: An extension to VS Code specifically for embedded systems.

Platform IO takes away a lot of the busy-work for you when it comes to compiling and installing your code on a device. A real time-saver when you're building stuff!

---

## Table of Contents

1. [What You'll Need](#1-what-youll-need)
2. [Installing PlatformIO](#2-installing-platformio)
3. [Opening the Project](#3-opening-the-project)
4. [Next Steps](#4-next-steps)


---

## 1. What You'll Need

- **Raspberry Pi Pico 2W:** Our micro-controller. Make sure it's the **2W** (wireless) version with the RP2350 chip.
- **USB data cable:** A _data-capable_ Micro-USB cable.
- **Computer:** Windows, macOS, or Linux. This guide uses Windows paths, but the steps are similar on other OSes.
- **_Blossom_ Light Assembly:**. Construction and wiring details are located in the [Blossom Assembly Guide](/docs/assembly.md).

## 2. Installing the Software

### Step 2.1: Install VS Code

Visual Studio Code ("VS Code") is where we'll be able to see and edit all of the project files, including the code. If you don't already have VS Code installed:

1. Go to [code.visualstudio.com](https://code.visualstudio.com/)
2. Download the installer for your operating system
3. Run the installer (accept all defaults)
4. Launch VS Code once installation completes

### Step 2.2: Install the PlatformIO Extension

We usually need a whole software stack in order to build things on a device. Platform IO is an extension to VS Code that collects together all the tools and code associated with different hardware platforms. We simply tell it we're developing on a Raspberry Pi Pico 2W, and it will grab everything we need to be Pico devs!

1. Open **Visual Studio Code**
2. Click the **Extensions** icon in the left sidebar (it looks like four squares)
3. In the search bar at the top, type **"PlatformIO IDE"**
4. Find the entry by **"PlatformIO"** (it should be the first result, with a little alien-head icon)
5. Click **Install**
6. Wait for the installation to complete — this may take a few minutes as it downloads the underlying toolchain

> **Note:** The first time PlatformIO installs, it may download several hundred megabytes of tools (compilers, debuggers, etc.). This is normal and only happens once.

### Step 2.3: Verify PlatformIO is Working

1. After installation, look at the left sidebar of VS Code. You should see a new **"PlatformIO"** icon (a little alien head) or a status bar with PlatformIO information.
2. Open the **PlatformIO Home** page by clicking the alien head icon, or by pressing `Ctrl+Shift+P`, typing **"PlatformIO: Home"**, and pressing Enter.
3. If you see the PlatformIO home screen, you're all set.

---

## 3. Opening the Project

1. In VS Code, click **File → Open Folder...** (or press `Ctrl+K Ctrl+O`)
2. Navigate to the `blossom_programmable_light_display` folder
3. Click **Select Folder**

VS Code will now open the project. PlatformIO will automatically detect the `platformio.ini` configuration file and set up the project environment. This .ini file has all the details about the hardware we're using (the Raspberry Pi Pico 2W) and the SDK we're building on top of (we're using the Arduino framework with the Earle Philhower core).

There's also a .pio directory in the project where PlatformIO will store everything it needs to make a build with our specific hardware. This folder will fill up with stuff when you first build the project. You won't need to worry about it; PlatformIO will keep that space organized.

---

## 4. Next Steps

Installation Complete! Let's get the rest of your environment set up. 

See the [Build and Upload Guide](build_and_upload.md) for help setting up your hardware.
