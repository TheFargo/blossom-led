# LED Technical Details: Blossom Programmable Light Display

We're artists painting with light, and our instrument of choice for this project is called the "SK6812RGBW-WS" - it's a tiny 5mm x 5mm wafer of electronics with four integrated LEDs. These things are _bright_ and _colorful_. Best of all, you can string them together and address them individually, creating incredible patterns.

In this guide, we'll look at these cool little LEDs, talk about how they work, and then talk about how we send instructions to them with our microprocessor!

---

## Table of Contents

1. [Introducing the SK6812](#1-introducing-the-SK6812)
2. [Adafruit's "NeoPixel" Rings](#2-adafruits-neopixel-rings)
3. [Talking to LEDs](#3-talking-to-leds)
4. [Taking Advantage of the Pico](#4-taking-advantage-of-the-pico)
5. [Newer LED Tech](#5-newer-led-tech)
6. [Power and Other LED Considerations](#6-power-and-other-led-considerations)
7. [Further Reading](#7-further-reading)

---

## 1. Introducing the SK6812

Not long after humanity mastered the secrets of the elusive blue LED, the race was on to shrink and string together LEDs into everything from decorative lighting strips to bright LED monitors. In the early 2000s engineers invented a "5050" LED package, so-named because it was 5.0 mm x 5.0 mm in size. Whole strips of these tiny lights could be easily manufactured. You could put an adhesive backing on them to place a thin strip of bright lighting wherever you needed it. Depending on how they were wired, you could control the RGB values of the whole strip at once.

But programmers and artists wanted to be able to make patterns with the lights! To do this, engineers needed to design "individually addressable" LEDs so that you can control the color and brightness of each individual light in the string. It seems crazy to control a whole line of lights with a single data out line from a microcontroller, but those mad lads did it! A generation of lights came out with names like WS2812B or SK6812. Rather than make people memorize alphabet soup, companies like Adafruit gave this technology a name: "NeoPixels."

We are interested, specifically, in the "SK6812" lights, invented by Dongguang Opsco Optoelectronics in 2015. These guys are special because they have FOUR LED lights integrated into the package: red, green, blue, and a special white LED that comes in various color warmths. For this project, I like the warmest color available, "Warm Sunlight." This looks the most like an incandescent light or candle flame at low intensities. The full chip name, "SK6812RGBW-WS" reflects this choice. Let's look at it!

![SK6812 Extreme Close Up](images/led-guide-6812.jpg)

What's this? There's a whole little logic circuit in there! Each of these lights have their own tiny controller embedded _right onto the chip_. It's not super sophisticated; all it does is either light up a specific color or pass the color information on to the next light. We'll talk about how that works in [part 3](#3-talking-to-leds).

You'll see four pads on the corners of the chip, the classic "5050" design for these things. I've labelled the pads for the SK6812: Power input (5V) and ground are on opposite corners. The other two corners are for "data in" and "data out." The idea is you can wire up a whole line of these pretty cleanly, having a shared 5V line on one side, the ground line on the other, and a data line snaking between each light. 

Speaking of lights, it's hard to see the individual LEDs when the package isn't lit up. The red, green, and blue LEDs are packed together on the right side of the image. Turn your attention to that big golden semicircular area at the top of the chip. That's the whole reason we chose this hardware! That yellow piece is a diffuser on top of a VERY bright pure-white LED. The yellow color is why the resulting light is a warm yellow shine instead of the bright blue-white we associate with colder LEDs.

From any distance, the white and color channels blend right together, especially if you've got a good diffuser over the light. But if you're up-close - such as when the device is sitting on your desk - the color LEDs and the white LED are visibly just a little offset from one another. In the case of the Blossom, this slight offset only adds to the visual. In the photo below, all sixteen chips are lit up with both red and white lights. Taken together, they make a kind of spiral pattern!

![The Blossom, photographed at low exposure so you can see the differences in the lights.](images/led-guide-spiral.jpg)

---

## 2. Adafruit's NeoPixel Rings

Individual LEDs are cool, but they're _tiny_. Without a heat-gun and an advanced workshop it's really hard to solder these. But the whole point of the 5050 form factor is to be able to easily manufacture these in strips or shapes or matrixes. A quick search for "SK6812 LED" on [Alibaba](https://www.alibaba.com/) will reveal hundreds of such products. My favorite designs come from [Adafruit](https://www.adafruit.com/), a New York based company focused on cool tech for hobbyists like us.

Adafruit uses the "NeoPixel" name for WS2812B designs (these only have the color LEDs, or "RGB") and the SK6812 designs (these are the ones with a dedicated white channel, "RGBW"). As discussed above, we're looking for the warmest white channel we can find, which brings us to this beauty:

* [Adafruit NeoPixel Ring 16x RGBW Warm White](https://www.adafruit.com/product/2854)

![Adafruit's NeoPixel 16 RGBW Ring.](images/led-guide-neopixel.jpg)

Adafruit has assembled 16 lights in a circle. The positioning of the white channel on the chip gives us a wonderful "outer ring" of white lights. The PCB is cut to the smallest possible size and we're given 6 connections, three of which I've labelled above. The power, ground, and data lines are (by design) 90-degrees apart around the circle. There's also a data-out line, for stringing multiple rings or strips of NeoPixels together (Adafruit's website has [Plans for making light-up glasses](https://learn.adafruit.com/celebration-spectacles) by connecting two rings this way.)

The outer diameter of the 16-light ring is 44.5mm (1.75"). That's almost the exact dimensions of a tea-candle, which gives us a library of decorative hardware to choose from. And we can address the white lights and colored lights independently, playing one pattern on the colorful "inner ring" while playing another on the bright "outer ring." It's like getting two rings of lights in a single hardware package! 

---

## 3. Talking to LEDs

You don't have to understand how the lights work to program pretty colors, but the technology that makes these lights "individually addressable" is pretty cool, so let's talk it out!

### Transmitting Data in Binary

The data line is binary, meaning it can only be "on" or "off" (technically, the voltage on the wire is either high or low.) Our goal is to turn little on/off pulses into color information. There are a lot of ways to do this, but our hardware uses signal timing. That is, a short-duration high pulse represents a 0 and a long-duration high pulse represents a 1. These pulses are spaced out very precisely (1.25 microseconds each!).

### From Binary to Colors

To convert the 1s and 0s (bits) into color information, we need to assign a handful of bits to each LED. 8 bits is known as a byte, and it's a digital way of storing a number between 0 (00000000) and 255 (11111111). Our hardware assigns one byte for each color LED. For instance, let's look at the red LED:

| Binary | Decimal | Hex | Result |
| :---: | :---: | :---: | :--- |
| 00000000 | 0 | 00 | Off |
| 01010000 | 80 | 50 | Dim red |
| 10010110 | 150 | 96 | Rich red |
| 11111111 | 255 | FF | Brightest red possible |

>[!TIP]
>Programmers use hexadecimal (a 16-digit number system, using A-F in addition to 0-9) as a shorthand way of referencing groups of bits. Each "digit" represents four bits, from 0 (0000) to F (1111). So, instead of saying "1111-1100," you can just say "FC."

Similarly, we can assign eight bits to the green LED. It's physically right next to the red one, so when the red one is going full blast (255), and the green one is going full blast (255), the result is bright yellow. We've also got a blue LED there, and - you guessed it - we give it eight bits of its own. If all three channels are equally bright, the resulting color looks white. (This is, in fact, what "white LEDs" are doing.)

We use a total of 24 bits to give three values for red, green, and blue. By mixing them, we can get all sorts of fun colors. You can see it gets a little cumbersome to type it out in binary every time:

| Binary | Decimal | Hex | Result |
| :---: | :---: | :---: | :--- |
| `00000000 00000000 00000000` | 0 0 0 | 000000 | Off |
| `01010000 00000000 00000000` | 80 0 0 | 500000 | Dim red |
| `11111111 00000000 00000000` | 255 0 0 | FF0000 | Bright Red |
| `00000000 11111111 00000000` | 0 255 0 | 00FF00 | Bright Green |
| `00000000 00000000 11111111` | 0 0 255 | 0000FF | Bright Blue |
| `11111111 11111111 00000000` | 255 255 0 | FFFF00 | Yellow |
| `11111111 11010111 00000000` | 255 215 0 | FFD700 | Gold |
| `11111111 10100101 00000000` | 255 165 0 | FFA500 | Orange |
| `11011010 01110000 11010110` | 218 112 214 | DA70D6 | Orchid / Light Purple |
| `10000000 00000000 10000000` | 128 0 128 | 800080 | Purple |
| `01101010 01011010 11001101` | 106 90 205 | 6A5ACD | Slate Blue |
| `01000000 11100000 11010000` | 64 224 208 | 40E0D0 | Turquoise |
| `00000000 11111111 11111111` | 0 255 255 | 00FFFF | Cyan / Aqua |
| `11111111 00000000 11111111` | 255 0 255 | FF00FF | Magenta / Fuchsia |
| `11111111 11111111 11111111` | 255 255 255 | FFFFFF | Brightest white possible |
