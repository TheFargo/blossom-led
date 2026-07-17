# Pysical Assembly Guide: Blossom Programmable Light Display

Building your own Blossom from scratch? Great idea! This is the perfect starter/intermediate electronics and woodworking project. All of the files you need are in this repository, and all of the hardware and skills you'll need are listed below.

---

## Table of Contents

1. [Bill of Materials](#1-bill-of-materials)
2. [Workbench Equipment](#2-workbench-equipment)
3. [Manufacturing the Case](#3-manufacturing-the-case)
4. [Prepping the Lights and Spindle](#4-prepping-the-lights-and-spindle)
5. [Prepping the Blossom Sculpture](#5-prepping-the-blossom-sculpture)
6. [Wiring the Pico 2W](#6-wiring-the-pico-2w)
7. [Final Assembly](#7-final-seembly)
8. [Next Steps](#8-next-steps)

---

## 1. Bill of Materials

| Item | Qty | Est. Price (USD) | Vendor Ideas | Notes |
| :--- | :---: | ---: | :--- | :--- |
| **Raspberry Pi Pico 2W** | 1 | $7.00 | [Adafruit](https://www.adafruit.com/product/6087) [Microcenter](https://www.microcenter.com/product/687384/raspberry-pi-pico-2-w) | Our heroic Microcontroller! Make sure to get the "2W." |
| **Adafruit NeoPixel Ring (16-pixel) RGBW** | 1 | $11.95 | [Adafruit](https://www.adafruit.com/product/2854) | Beautiful pre-assembled array of RGBW LEDs. I prefer warm-white. |
| **Capiz-Shell Decorative Lotus Art** | 1 | ~$10.00+ | (See notes) | Any decorative tea-light holder will work; See notes below for sourcing the beautiful capiz-shell assembly shown.  |
| **50mm Frosted-Glass Cabochon** | 1 | ~$1.50+ | [Amazon](https://www.amazon.com/dp/B07JKZP1Z6) | This acts as a diffuser to bounce our LED lights around; a 50mm translucent acyrlic circle is an inexpensive alternative. |
| **3mm (1/8-in) Basswood or Birch Plywood** | 1 | ~$4.00 | Local Lumber / Craft Store | You'll need roughly 8"x12" of wood for laser cutting the case. |
| **Micro-USB Cable (3ft to 6ft)** | 1 | ~$3.00 | Generic / Amazon [Example](https://www.amazon.com/Charging-Transfer-Android-Trustable-MYFON/dp/B098DW7485/) | For power and programming. Ensure it's a data cable, not just power! |
| **5V 1A USB Power Adapter** | 1 | ~$8.00 | Generic / Amazon [Example](https://www.amazon.com/Certified-Charger-Universal-Portable-Adapter/dp/B017TXGM4I/) | Standard phone charger wall brick to power the Blossom independant of a computer. |
| **M2 Screws (6mm)** | 4 | $.40 | [Amazon](https://www.amazon.com/HVAZI-Metric-Notebook-Computer-Assortment/dp/B075C6C4YR/) / [McMaster-Carr](https://www.mcmaster.com/91698A202/) [MicroConnectors](https://www.microconnectors.com/assorted-laptop-screws-set-250-pcs-scw-250lp/)| For securing the Pico 2W to the wooden base. |
| **#6 Wood Screws (1/2in)** | 4 | $.75 | [Amazon](https://www.amazon.com/TPOHH-Stainless-Phillips-Threaded-5x12SS18-8/dp/B092Q87W39/) / [McMaster-Carr](https://www.mcmaster.com/90031A552/) | For fastening the two halves of the base enclosure. |
| **Rubber Feet (Self-Adhesive)** | 3 | $1.00 | [Adafruit](https://www.adafruit.com/product/550) [Amazon](https://www.amazon.com/dp/B074PXFWPK/ref=twister_B092W7TL7Y) | These little guys really elevate your build. (Looks directly at camera.) |
| **TOTAL ESTIMATED COST** | | **~$48.00** | | *Excludes workshop consumables (solder, wire).* |

>[!TIP]
>**Sourcing a Tea-Light Holder**:  
>The capiz-shell design pictured here is available from [World Market](https://www.worldmarket.com/p/capiz-20-petal-lotus-tealight-candle-holder-119956.html). These are crafted in the Philippines from local materials and look great. Anything designed for a 4cm tea-light candle should work. Try to find something with a lot of translucent surfaces for the light to play off of. Look around and see what "Blossoms" for you!

---

## 2. Workbench Equipment

To construct this project from scratch, you'll need the skills and equipment to laser-cut and prepare wood, and to solder wires onto circuit boards. If you've never done those things before, this is a _great_ project for getting started! Here's what you should have on-hand:

1. **A 5W (or Greater) Laser Engraver/Cutter.** For the best results, you'll want an air assist and a honeycomb workbench panel. A more powerful laser will cut the project faster, but a little 5W will do. 

2. **Wood Prep Materials.** Wood glue and clamps are essential, but for a really pro look and feel you'll also want sandpaper (200 and 400 grit), your color choice of stain, and your choice of finish (I'm a big fan of Danish Oil).

3. **A Soldering Iron Station.** Work in a well-ventilated area, preferably with a heat-resistant mat to protect your work surface. We're only soldering 6 connections, but they're very small: a magnifying glass and "helping hands" to hold the material will really help. 

4. **Adhesive.** You'll want a way to attach the wooden parts of the Blossom to the art piece - little bit of Gorilla Glue will do the trick here. Optionally, a hot-glue gun is terrific for attaching and running hidden wires.

5. **(Optional) Metal-Drilling Equipment.** If you need to run wiring through your lighting display, you may want to drill a hole. _Use caution when drilling metal!_ Wear gloves and goggles, secure the art well, and use a stepped drill bit. Blossom needs only three small wires, so rather than risk damaging your art (or yourself!), you may just want to discretely run the wires.

---

## 3. Manufacturing the Case

The files you need for cutting and engraving the wooden case and lighting spindle are located in the `\enclosure` folder.

![The Enclosure Before Prep and Assembly](images/laser_cut_parts.jpg)

Essentially, we're cutting out a "six-pack" of wooden disks. We'll glue three of these together to form the bottom half of the base with pre-printed instructions. The other three layers will be glued together to form the top half of the base, which holds the heart of the project: the blossom artwork, the lighting spindle, and the micontroller. Once assembled and wired, we will screw both halves together to enclose the hardware and wires in a small hollow cavity.

1. **Cut the Sandwich.** Here's the text.

2. **Mayonaise.** Yeah.