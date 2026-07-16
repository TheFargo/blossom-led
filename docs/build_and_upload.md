# Build and Upload Guide: Blossom Programmable Light Display

This guide walks you through using Platform IO to build and upload the firmware and file system into the **Blossom**.

Be sure you've followed the steps in the [Installation Guide](installation.md) and have VS Code and Platform IO installed with the project open. 

---

## Table of Contents

1. [Understanding the Pico's Flash Memory](#1-understanding-the-picos-flash-memory)
2. [Putting the Pico into "BOOTSEL" Mode](#2-putting-the-pico-into-bootsel-mode)
3. [Building and Uploading Code with PlatformIO](#3-building-and-uploading-code-with-platformio)
4. [Uploading the Filesystem (LittleFS)](#4-uploading-the-filesystem-littlefs)
5. [When to Update What](#5-when-to-update-what)

---

## 1. Understanding the Pico's Flash Memory

The Pico2W we're using for this project has 4MB of onboard Flash memory. Flash is programmable memory that stays put even when the power is off. We're going to divide it into two spaces:

    ┌─────────────────────────────────┐
    │  4MB Onboard QSPI Flash Memory  │
    │  ───────────────┬─────────────  │
    │    Firmware     │  File System  │
    │ (Compiled Code) │ (HTML, Fonts) │
    └─────────────────┴───────────────┘

All of the source code you see in the project will get compiled into machine language, a complete list of instructions for everything the device does as soon as you power it on. The Pico is designed for this and has a neat "execute in place" (XIP) feature where the CPU can run the code straight from the flash without copying it anywhere first. 

We'll build and upload our code in step 3.

We also set aside a chunk of the Flash memory to be used as a file system. This bit of flash will act a lot like a computer's hard drive - we can read files from it and write to it as needed. We use a library of tools for this called "Little FS." We need a file system because our Blossom acts as its own little webserver: It can serve pages from this file system, and the fonts your browser needs to display them. 

We'll upload the file system in step 4. 

## 2. Putting the Pico into "BOOTSEL" Mode

Our little Pico 2W has two different modes: When you turn it on it enters "Normal execution mode," where it looks for a program in its flash memory and starts following instructions. If you want to upload something to the device, you need to put it into "BOOTSEL" mode. This turns the Pico into basically a little USB drive that your computer can just drop files into.

### Using the BOOTSEL Button

To enter BOOTSEL mode, connect a data USB cable to your PICO, hold down the little white button, and plug the other end of the USB cable into your computer. You'll hear the "plink-plink" of a new USB device being mounted, and on most systems a window will pop-up allowing you to drag-and-drop code onto the device. You can release the button once it's booted.

### When do you do this?

The very first time you're uploading code to a new Pico, you'll probably have to manually click the button to get it into BOOTSEL mode. For every build afterwards, PlatformIO _should_ be able to handle the rebooting and uploading all on its own.

However, if PlatformIO ever reports that it can't find or can't reboot the hardware, you might have to do it yourself. Give it the ol' one-finger salute and it'll be ready for new code! 

## 3. Building and Uploading Code with PlatformIO

If you look closely at the very bottom of your VS Code window, you'll see a new little group of icons, including a checkmark and a right-arrow.

![PlatformIO Toolbar](images/PlatformIO_Tools.png)

These are some handy tools for working with external hardware. When it comes to building and installing code, we're interested in the checkmark and the arrow:

### PlatformIO: Build (the Checkmark)

When you click the checkmark, PlatformIO will go through everything your code needs to run on your target platform, download anything that's missing, and compile it all together. In your terminal window you'll see the results: If you have any problems you'll get an error (hopefully a helpful one), otherwise you'll see a SUCCESS message and a little time-stamp of how long it took. 

The first build might take a little bit, since PlatformIO has to go out and grab all the libraries we use for things like the webserver or operating WiFi. It'll go faster in the future, because you'll only recompile the files that changed. 

### PlatformIO: Upload (the Right Arrow)

When you click the arrow, PlatformIO goes through the build process (just like above), but when it's done it uploads the compiled code right into your device and reboots it for you. This is a huge time-saver while you're trying to iterate! In your terminal you'll see progress bars fill up as PlatformIO loads your code into the flash memory, then double-checks it to make sure it wrote correctly. This will take a few moments.

If you ever get red error messages while it's trying to upload, you may need to manually put your device into BOOTSEL mode for PlatformIO to do its thing (see above). You'll almost definitely ned to do this the first time you upload code to your device. 

## 4. Uploading the Filesystem (LittleFS)

For this project, we also set aside some flash memory for a file system. We use this to store HTML pages and the fonts used to display them. The Blossom acts as its own webserver and sends those webpages to anyone who connects - that way, people can play with the Blossom without downloading any special software!

In your main project directory you'll see a `data/` folder. The contents of this folder will be uploaded to the file system. 

Unfortunately, there's not a nice toolbar button to upload the file system for us, so we have to do it with a command line. If you don't have a terminal open, there's at least a button for that: A little to the right of the Build and Upload buttons, you'll see a square icon with a little prompt ">_" in it. Press this button to open up a terminal, then inside that terminal type:

    pio run --target uploadfs

If that gives a "command not found" error, try the full program name:

    platformio.exe run --target uploadfs

Still can't find it? Try typing in the full path, which will look different on every machine. On Windows it might look like:

    C:\Users\yourname\.platformio\penv\Scripts\platformio.exe

If your computer still can't find the PlatformIO tools you'll have to configure the path, which is a little outside the scope of this guide.

A successful file system upload will look very much like a source code update: The flash will be written, you'll see a bar chart of progress, and then it'll be checked and your device will reboot. Just like updating the source code, if PlatformIO can't seem to find or reboot your Pico, you may have to put it in BOOTSEL mode (see above.)

## 5. When to Update What

You won't have to re-upload everything every time you want to make changes. The Pico considers the Flash memory as two distinct spaces. So, if you don't change anything in the `/data` folder, the file system is unchanged, and doesn't need to be re-uploaded. Similarly, if you're just changing webpages in the `/data` directory and haven't made any changes to code, there's no need to rebuild and re-upload the firmware.

However, if you DO change both at once, be sure to upload the file system FIRST, and then compile and upload the code. This way the code knows where the new files are located. 

---

Congratulations! Configuring this toolchain is one of the most difficult parts of setting up a new project. Now you're ready to do some amazing things with Blossom and your Pico 2W!