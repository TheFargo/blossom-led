# Instructions: Blossom Programmable Light Display

![Blossom Logo](/docs/images/blossom_logo.png)

## It's Time to Blossom!

May this gift bring warmth, light, peace and joy to your environment.

A brief version of these instructions are engraved onto the bottom of your Blossom. Have fun!

---

## Table of Contents

1. [Package Contents](#1-package-contents)
2. [How Blossom Works](#2-how-blossom-works)
3. [Setup](#3-setup)
4. [Offline Mode](#4-offline-mode)
5. [Playing Animations](#5-playing-animations)
6. [Playing Meditations](#6-playing-meditations)
7. [Factory Reset](#7-factory-reset)
8. [Troubleshooting](#8-troubleshooting)

---

# 1. Package Contents

* 1 Blossom Programmable Light Display
* 1 Micro-USB to USB-A Cable
* 1 Region-Appropriate USB-A Charging Block

Plug your Blossom in and make sure all the lights turn on. Your Blossom can be plugged directly into a wall outlet via the charging block, or it can be connected to any USB port (such as your laptop or desktop computer.)

---

# 2. How Blossom Works

Your Blossom is more than a bunch of pretty lights! Each Blossom is a little tiny computer that runs its own webserver and hosts its own webpages. That means anyone with a web browser can connect to Blossom and play with it in real-time. If you connect Blossom to your Wi-Fi, you can control it from any machine on your network. 

![How Blossom Works](/docs/images/instructions_connect.png)

If you don't want to give Blossom any Wi-Fi credentials, it can still be used in ["offline mode"](#4-offline-mode). When offline, the only way to control Blossom is to connect to it directly. 

>[!TIP]
>You can take Blossom camping or off-grid! Even when there's no Internet or Wi-Fi nearby, you can still use offline mode to control Blossom from any device.

---

# 3. Setup

When you first power on the Blossom, it will enter *Setup Mode*. While in Setup Mode, the Blossom acts as a Wi-Fi hotspot. You can connect to it and configure it from any Wi-Fi enabled device, such as your phone or laptop.

**During Setup, the "Status" indicator on the bottom of the Blossom will slowly blink.**

1. *Connect:* Go to the Network Settings of any device - the place where you would select a Wi-Fi network. If you're anywhere near the Blossom, you will see `Blossom_Setup` as an available network. Select it and click 'Connect!'

2. *Configure:* A setup page should pop up immediately. Depending on the OS of the device you're using and its default web browser, this "captive" webpage might show up instantaneously, or it may take a moment. If it does not appear at all, open up a web browser and Blossom should jump in.

3. *Credential...ize!* Blossom will display a list of nearby networks and their signal strength. Choose your network and type in the Wi-Fi password. Click `Connect to Network.`

Blossom will now reboot and attempt to connect to your network with the given credentials. If the password is correct and the network nearby, Blossom will enter Connected Mode.

**During Connected Mode, the "Status" indicator on the bottom of the Blossom will remain on.**

Connect to `http://blossom.local` from any device on the same Wi-Fi network to play animations and meditations!

---

# 4. Offline Mode

If you are nowhere near a Wi-Fi connection or want to control Blossom without using your network password, all pf Blossom's features are still available! Simply connect to the `Blossom_Setup` network as described above to bring up the captive Setup page. Click on `Offline Mode` to use the animation and meditation features.

While in Offline Mode, your device won't be connected to any other Wi-Fi network.

---

# 5. Playing Animations

Text text

**To Play Animations, go to http://blossom.local from any connected web browser.**

More information about how to configure and play animations can be found in our [Animation Guide.](/docs/animation_guide.md).

---

# 6. Playing Meditations

Text text

**To Play Meditations, go to http://blossom.local from any connected web browser.**

More information about mindfulness and the meditation technique used here can be found in our [Meditations Guide.](/docs/meditations.md).

---

# 7. Factory Reset

A "Reset" button is hidden inside the Blossom, accessable via a pinhole on the bottom of the sculpture. Look for the Reset location just below the Status indicator light.

To initiate a factory reset (wiping all network credentials), use a paperclip to _press and hold the Reset button for 5 seconds_. You will be able to feel the button click, and during the 5-second countdown the Status light will blink rapidly.

After 5 seconds, the device will restart in Setup Mode with no Wi-Fi credentials saved.

---

# 8. Troubleshooting

* *One or More Lights are Out.* Blossom's low-power ambient display should run wihtout fail for thousands of hours, but sometimes LEDs give out. If several are out, it indicates that a broken LED is also interrupting the data line (see [Blossom's LED Technical Details](/docs/led_technical_details.md)). The easiest fix is to replace the defective LED ring.

* *The Ring Does Not Light Up at All.* If the Status light is operating but the LED lights are dark, this indicates a bad wiring connection. Open everything up and check the connections. If the Status light is ALSO dark, your device is getting no power; ensure that your USB cable is plugged in and powered.

* *Blossom_Setup Does Not Appear as a Network.* If you are trying to configure the Blossom and don't see the `Blossom_Setup` network, your Blossom may already be connected to something. Make sure the Status light is blinking to indicate connected mode. Try power-cycling the Blossom. If you see a solid light but wish to reconfigure the Blossom, do a [Factory Reset](#7-factory-reset).

* *Cannot Connect to Blossom.Local.* Make sure Blossom is connected to the Wi-Fi by checking the Status light - if it's blinking, you'll need to re-connect it. If the light is on and you still don't see the Blossom, ensure that your devices are using the same network. Try power-cycling the Blossom to get a fresh connection. If you are using Wi-Fi extenders, not that the cheap ones may not forward local webpages.

* *Please contact me with other bug reports.*

