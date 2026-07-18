# LED Technical Details: Blossom Programmable Light Display

We're artists painting with light, and our instrument of choice for this project is called the "SK6812RGBW-WS" - it's a tiny 5mm x 5mm wafer of electronics with four integrated LEDs. These things are _bright_ and _colorful_. Best of all, you can string them together and address them individually, creating incredible patterns.

In this guide, we'll look at these cool little LEDs, talk about how they work, and then talk about how we send instructions to them with our microprocessor!

---

## Table of Contents

1. [Introducing the SK6812](#1-introducing-the-SK6812)
2. [Adafruit's "Neopixel" Rings](#2-adafruits-neopixel-rings)
3. [Talking to LEDs](#3-talking-to-leds)
4. [Taking Advantage of the Pico](#4-taking-advantage-of-the-pico)
5. [Newer LED Tech](#5-newer-led-tech)
6. [Power and Other LED Considerations](#6-power-and-other-led-considerations)
7. [Further Reading](#7-further-reading)

---

## 1. Introducing the SK6812

Not long after humanity mastered the secrets of the elusive blue LED, the race was on to shrink and string together LEDs into everything from decorative lighting strips to bright LED monitors. In the early 2000s engineers invented a "5050" LED package, so-named because it was 5.0 mm x 5.0 mm in size. Whole strips of these tiny lights could be easily manufactured. You could put an adhesive backing on them to place a thin strip of bright lighting wherever you needed it. Depending on how they were wired, you could control the RGB values of the whole strip at once.

But programmers and artists wanted to be able to make patterns with the lights! To do this, engineers needed to design "individually addressable" LEDs so that you can control the color and brightness of each individual light in the string. It seems crazy to control a whole line of lights with a single data out line from a microcontroller, but those mad lads did it! A generation of lights came out with names like WS2812B or SK6812. Rather than make people memorize alphabet soup, companies like Adafruit gave this technology a name: "Neopixels."

We are interested, specifically, in the "SK6812" lights, invented by Dongguang Opsco Optoelectronics in 2015. These guys are special because they have FOUR LED lights integrated into the package: red, green, blue, and a special white LED that comes in various color warmths. For this project, I like the warmest color available, "Warm Sunlight." This looks the most like an incandescent light or candle flame at low intensities. The full chip name, "SK6812RGBW-WS" reflects this choice. Let's look at it!

![SK6812 Extreme Close Up](images/led-guide-6812.jpg)

