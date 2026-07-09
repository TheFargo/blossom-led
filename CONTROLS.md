# Control Page for Blossom Colors and Animations

# Container 1: Logo and Mode Indicator/Selector

1. "Blossom" Logo
2. Text: "Now Playing:"
3. Button: Label = Current Lighting Pre-set or Game Mode.
    Clicking the button will open up a list of presets or modes to choose
    Default: Button says "Warm Flame", the default lighting preset.

Container 1 remains consistent throughout the application.
All containers below change to reflect the mode.
The below containers are all specific to color animations,
including the "Warm Flame" default for the device.

# Container 2: Color Channel

Here the user defines the base colors for the 16 pixels, before any animation
tracks are applied.

1. Label: Color
2. Slider: Primary (255 positions, representing hue)
3. Slider: Spread (255 positions, representing color variation.)
  A spread of 0 means that all pixels will have the same color.
  A spread of 255 means that pixels will use the entirety of the spectrum.
    Lower spreads allow for more subtle effects:
    Primary color Orange with a low spread will create a spectrum from yellow to red.
  Somewhere - ideally flanking this control - we should show the two resulting color
    extremes. Users can quickly iterate by adjusting the color and spread sliders.
4. Slider: Brightness (255 positions, representing color LED brightness levels.)
  Brightness levels are *relative* to LED_BRIGHTNESS constant in code! That's a hard cap!
5. Radio Buttons: Color Spread (Three options available:)
    - Random (The resulting color spread is randomly distributed among the pixels)
    - Ordered (The color spread is placed in order from first pixel to last)
    - Looping (Color spread split in two, using half the pixels to reach destination
        color, and half to transition back, erasing the "seam.")

# Container 3: White Channel 

These particular neopixels have a warm white LED in addition to the color channels.
In this section, users can definte how bright and varied they want these pixels to be. 

1. Label: Sparkles
2. Slider: Brightness (255 positions, representing white LED brightness levels.)
  Brightness levels are *relative* to LED_BRIGHTNESS constant in code! That's a hard cap!
3. Slider: Spread (255 positions, representing brightness variation.)
  Same idea here as color spread, but applied to brightness:
  A spread of 0 means all the white lights are the same brightness.
  A spread of 255 means the lights will vary from full bright to full dark. 
  Again, the LED_BRIGHTNESS cap is respected as the max value in these calculations.
4. Radio Buttons: Sparkle Spread (Three options available:)
    - Random (The resulting white spread is randomly distributed among the pixels)
    - Ordered (The white spread is placed in order from first pixel to last)
    - Looping (White spread split in two, using half the pixels to go from min to 
      max brightness, and the other half to transition back, creating a seamless loop.)

# Container 4: Flicker Animation

The Flicker animation will use a randomized noise track to move a pixel between the two colors (or brightnesses) of its spread. 

1. Label: Flicker
2. Checkbox: Color (Applies animation to the color pixels)
3. Checkbox: White (Applies animation to the white pixels)
4. Slider: Speed (255 positions, representing flicker speed)
5. Slider: Amplitude (255 positions, representing noise amplitude)
  At low amplitudes, pixels will stay close the the primary color/brightness.
  At high amplitudes, the value of the noise is multiplied, but capped at the spread.
    This means pixels will stay at extremes longer. 
6. Radio Buttons: Synchonicity (Three options availble:)
    - Random (Every pixel uses its own noise pattern)
    - Ordered (Pixels follow the noise pattern in order, so it moves along the ring)
    - Looping (Pixels are split in two, half following the pattern, half reflecting it)

# Container 5: Pulse Animation

The Pulse animation uses a sine-wave track to smoothly cycle a pixel between the two colors (or brightnesses) of its spread.

1. Label: Pulse
2. Checkbox: Color (Applies animation to the color pixels)
3. Checkbox: White (Applies animation to the white pixels)
4. Slider: Speed (255 positions, representing Pulse speed)
5. Slider: Amplitude (255 positions, representing noise amplitude)
  At the halfway point, the pixel will animate exactly from one extreme to another.
  At high amplitudes, the value of the curve is multiplied, but capped at the spread.
    This means pixels will stay at extremes longer. 
6. Radio Buttons: Synchonicity II (Three options availble:)
    - Random (Every pixel uses its own sine curve pattern)
    - Ordered (Pixels follow the sine curve in order, so it moves along the ring)
    - Looping (Curve is squashed to fit entirely in the number of LEDs)

# Container 6: Spin Animation

After all other effects are applied to the pixels in order, the pixels can be "shifted" by one or more pixels clockwise or counterclockwise each frame.

1. Label: Spin
2. Checkbox: Color (Applies animation to the color pixels)
3. Checkbox: White (Applies animation to the white pixels)
4. Slider: Speed (255 positions, from -128 to 128, representing spin speed and direction)
