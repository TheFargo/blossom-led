# Animation Guide: Blossom Programmable Light Display

Blossom's animator is designed around two ideas:

1. Make it iterative to play different colors and patterns
2. Keep animations state-free and mathematical

The first idea keeps Blossom friendly to play with: artists can change colors or flicker patterns just by sliding their thumb along the controls, watching the results in real-time. The second ensures that Blossom's animations are infinite and can be described quickly by sending just a few parameters over the local network.

The resulting animations are generally ambient loops that emulate a colorful, magical flame.

https://github.com/user-attachments/assets/a1dfe79e-bccb-4c35-a51d-3a526a6722f9

---

## Table of Contents

1. [Sixteen Special Sparkly Lights](#1-sixteen-special-sparkly-lights)
2. [Setting Colors and Sparkles](#2-setting-colors-and-sparkles)
3. [Color and Sparkle Spreads](#3-color-and-sparkle-spreads)
4. [Flickers, Pulses, and Spins](#4-flickers-pulses-and-spins)
5. [Saving Presets](#5-saving-presets)
6. [Controlling Blossom from Any Connected Device](#6-controlling-blossom-from-any-connected-device)

---

# 1. Sixteen Special Sparkly Lights

Blossom's ring of light is composed of 16 RGBW LEDs. "RGBW" ("Red, Greeen, Blue, White") means that each light can be set to any color (the "red, green, and blue" part) in addition to having an additional white light on top of it. 

>![Tip]
> Take a deep-dive into the LEDs we're using and how they work in Blossom's [LED Technical Details](/docs/led_technical_details.md).

Blossom's separate ring of white lights gives artists the opportunity to play one animation across the color LEDs while playing a _different_ animation on the white lights. In the interface, the two channels are treated completely separate, named *colors* and *sparkles.*

---

# 2. Setting Colors and Sparkles

When we set the Blossom's color, we provide it with a prmary color and a range, resulting in a spectrum. Imagine a color wheel... or better yet, look at this picture of one I drew:

![The Color Wheel, with primary colors (red, green, blue) highlighted](/docs/images/animation_colorwheel.png)

Picking a color is like picking a direction, and selecting a range is like selecting how big of an arc you want to draw centered on that direction.

- A range a 0 means that the lights won't vary from the selected color at all.
- Selecting a small range will cause the lights to fluctuate in a narrow color band centered on the selected color. For example, selecting orange and a low range will generate a nice warm spectrum from red to yellow.
- Moving the range slider to maximum means that Blossom's lights can cycle through the entire spectrum, with the chosen color determining the spectrum's "center."

The white "Sparkle" lights that encircle the blossom cannot change color, only brightness. To set up sparkle animations, select a primary brightness and a range.

- A range of 0 means that all sprkle lights will have the same brightness
- A maximum range means the lights will vary from full brightness to completely off, with the selected brightness acting as the "centerpoint."

Although Blossom's animation interface will preview colors and ranges, the best way to see results is on the Blossom itself. When connected, it will respond instantly to changes on the web interface. 

---

# 3. Color and Sparkle Spreads

When selecting a color spectrum or sparkle brightness, you'll note a "spread" option. This determines how the lights will display the selected range.

>![TIP]
>To properly see spreads, make sure no animations are playing (deselect any flickers, pulses, or spins on either channel.) When looking at color spreads, the sparkle lights can be overpowering, so it's best to turn them off completely. (Off means brightness of 0 in addition to 0 spread!)

- *Unison:* All of the lights display the selected color or brightness. 
- *Ordered:* The range is spread equally across all 16 lights.
- *Looping:* Half the lights display the range, which is mirrored on the other half to create a seamless "loop."
- *Random:* The range is "randomly" distributed among the lights. 

>![TIP]
>"Random" is actually a mathematical cris-crossing pattern that spreads out the lights all around the ring.

---

# 4. Flickers, Pulses, and Spins

Three different animations (or any combination thereof!) can be applied to the lights based off of the color and brightness spreads defined above. 

![Illustrating a Pulse vs a Flicker pattern on a color gradient.](/docs/images/animation_pulse_flicker.png)

- A *Pulse* is a smooth sine-wave pattern alternating between the two ends of the range.
- A *Flicker* is a random noise pattern peaking toward either end of the range.

Artists can adjust the speed and amplitude of these patterns to get the desired effect.

The _Synchronicity_ of the animations determines how they are played out across the lights. As with color or brightness spreads, "Unison" plays all the lights together, "Ordered" plays animations in a loop around the ring, and "Random" spreads the animation across all the lights so they appear to be acting on their own.

- The *Spin* animation is applied last, shifting the lights clockwise or counter-clockwise around the ring.

---

# 5. Saving Presets

Clicking "Save Preset" at the bottom of the animation interface allows artists to name and save their favorite animations. From this same interface, the "Make Default" checkbox makes the selected preset the default whenever the device is powered on - even if it's unable to connect!

The "Now Playing" area at the top of the animation interface lists the currently playing preset. Clicking here opens up a list of saved presets, with the default marked by a star.

---

# 6. Controlling Blossom from Any Connected Device

The Blossom is built to be controlled from any device on your network! If you're a maker, tinkerer, or engineer into the whole "Internet of Things," you can explore other ways of controlling Blossom aside from the provided web interface. The [TO DO! API GUIDE](/docs/api.md) explains it all.

