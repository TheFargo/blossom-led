###############################################################################
###                              PIO PROGRAMS                               ###
###                                                                         ###
### The cool new RP2350 chip that powers your computer has tiny little      ###
### "engines" built into it, called "Programmable Input/Output," or "PIOs." ###
### These are almost like little mini-processors that can be running their  ###
### own chunks of code alongside the CPU. Each one has four itty-bitty      ###
### "state machines" that can operate independently. They're great for      ###
### sending out perfectly-timed signals at differing frequencies while the  ###
### main CPU is doing the more "important" stuff.                           ###
###                                                                         ###
### PIO state machines are simple devices that can only do a few basic      ###
### instructions. They have to be programmed in a special "assembly-like"   ###
### syntax that you'll see below. They can't do a lot, but they can do it   ###
### fast!                                                                   ###
###                                                                         ###
### The examples below demonstrate PIOs that control the timing of our DVI  ###
### signal, that send data to your LED monitor, or control the framerate    ###
### of neopixel jewel animations.                                           ###
###############################################################################

@rp2.asm_pio()
def neopixel_director():
    """
    A commandable animation timekeeper. Receives new delay values from the CPU
    via its TX FIFO, and signals "send out next frame" to the CPU via its RX FIFO.
    """
    pull(block)           # Director will not start until it gets a delay value
    mov(y, osr)           # Move the delay value from the OSR to y.
    wrap_target()
    mov(x, y)             # Just in case: Pre-load x with the current delay
    pull(noblock)         # If anything is in the TX-FIFO, move it to OSR (change framerate on the fly)
                          # Note: If TX-FIFO is empty, it grabs X. Which is why we pre-load X.
    mov(y, osr)           # Move the OSR to Y. 
    mov(x, y)             # Set our counter to that current delay value
    label("delay_loop")
    jmp(x_dec, "delay_loop") # The counter itself, decrementing X until it hits 0
    mov(isr, y)           # Copy the contents of Y to ISR (our timer value - any number will do)
                          # When the CPU sees a number there, it knows the next frame is due.
    push()                # Push the number to the RX-FIFO so the CPU can see there's data there
    wrap()

@rp2.asm_pio(sideset_init=rp2.PIO.OUT_LOW, out_shiftdir=rp2.PIO.SHIFT_LEFT, autopull=True, pull_thresh=32)
def neopixel_projector():
    """
    Waits for a stream of colors from the main program and sends them to the
    NeoPixel with precise timing. WS2812B timing constants: Each "pulse" is
    0.125 microseconds (us). Total signal is 10 pulses / 1.25 us.
    Run this machine at 8MHz (freq=8000000) for microsecond-perfect timing!
    """
    # To send a "1", we need a high pulse of 7 cycles (T1 + T2) and a low pulse of 3 cycles (T3).
    # To send a "0", we need a high pulse of 2 cycles (T1) and a low pulse of 8 cycles (T2 + T3).
    T1 = 2; T2 = 5; T3 = 3 # Cycle delays. Combine timings for 10 cycles per bit.
    wrap_target()
    label("bitloop")
    out(x, 1)               .side(0) [T3 - 1]
    jmp(not_x, "do_zero")   .side(1) [T1 - 1]
    label("do_one")
    jmp("bitloop")          .side(1) [T2 - 1]
    label("do_zero")
    nop()                   .side(0) [T2 - 1]
    wrap()


###############################################################################
###                            CLASS DEFINITIONS                            ###
###                                                                         ###
### We programmers are a neat and tidy bunch (at least, the good ones are.) ###
### A great rule of thumb for good programming is to hide away everything   ###
### that people don't need to see. You know: "Hide your mess." The proper   ###
### technical term for this is "Encapsulation," that is, you roll up all    ###
### the data and functions you need to do a certain job together.           ###
###                                                                         ###
### One example is below. We define a class called "NeoPixelBootAnimation." ###
### It has all the instructions the computer needs in order to start, play, ###
### and stop a whole animation for the neopixel jewel on the front of your  ###
### computer. It can handle all the little details that only matter to the  ###
### animation - like how bright the pulse should be or how what animation   ###
### frame we're on or the data for the different animation frames. We don't ###
### want all that clutter sitting around! So we wrap it all into one object,###
### and expose commands to initialize it, update it, stop it, etc. All the  ###
### code to handle setting it up to begin with and cleaning it up at the    ###
### end is nicely tucked away inside the object, neat and tidy!             ###
###############################################################################

class NeoPixelBootAnimation:
    """
    An encapsulated object to manage the entire neopixel jewel boot anim sequence.
    This is a hardcoded spiral->pulse->fade sequence, but it demonstrates the
    flexibility of the neopixel_director and neopixel_projector state machines.
    Neopixel animations are relatively low-priority and optimized for low CPU use.
    """
    ### CONSTANTS: HARDWARE ###
    DIRECTOR_FREQ = 50000
    PROJECTOR_FREQ = 8000000
    NUM_PIXELS = 7
    ### CONSTANTS: TIMINGS ###
    SPIRAL_FPS = 10
    PULSE_FPS = 50
    FADE_FPS = 60
    ### CONSTANTS: WHITE PULSE ###
    PULSE_MAX_BRIGHTNESS = 40
    PULSE_STEP = 3
    ### CONFIGURATION: ANIMATION FRAME DATA ###
    # We pre-package the animation frames as neopixel data ready to go,
    # saving some caluclations during boot. The CPU grabs a frame, fires it off
    # to the projector, and can get back to whatever it was doing. 
    PRECOMPUTED_SPIRAL_FRAMES_DATA = (
        # Frame 0: Pixel 2 (Cyan) lights up.
        (0x00000000, 0x00000000, 0x0C000C00, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
        # Frame 1: Adds Pixel 3 (Blue).
        (0x00000000, 0x00000000, 0x0C000C00, 0x00000C00, 0x00000000, 0x00000000, 0x00000000),
        # Frame 2: Adds Pixel 4 (Deep Purple).
        (0x00000000, 0x00000000, 0x0C000C00, 0x00000C00, 0x00070A00, 0x00000000, 0x00000000),
        # Frame 3: Adds Pixel 5 (Red).
        (0x00000000, 0x00000000, 0x0C000C00, 0x00000C00, 0x00070A00, 0x000C0000, 0x00000000),
        # Frame 4: Adds Pixel 6 (Red-Orange).
        (0x00000000, 0x00000000, 0x0C000C00, 0x00000C00, 0x00070A00, 0x000C0000, 0x040C0000),
        # Frame 5: Adds Pixel 1 (Pure Yellow).
        (0x00000000, 0x0C0C0000, 0x0C000C00, 0x00000C00, 0x00070A00, 0x000C0000, 0x040C0000),
        # Frame 6: Adds Pixel 0 (Green).
        (0x0C000000, 0x0C0C0000, 0x0C000C00, 0x00000C00, 0x00070A00, 0x000C0000, 0x040C0000))
    PRECOMPUTED_FADE_FRAMES_DATA = (
        # Frame 0 (95% brightness)
        (0x0C000000, 0x0B0B0000, 0x0B000B00, 0x00000B00, 0x00060900, 0x000B0000, 0x030B0000),
        (0x0C000000, 0x0A0A0000, 0x0A000A00, 0x00000A00, 0x00050800, 0x000A0000, 0x030A0000),
        (0x0C000000, 0x0A0A0000, 0x0A000A00, 0x00000900, 0x00050800, 0x00090000, 0x03090000),
        (0x0C000000, 0x09090000, 0x09000900, 0x00000900, 0x00050700, 0x00090000, 0x03090000),
        (0x0C000000, 0x08080000, 0x08000800, 0x00000800, 0x00040600, 0x00080000, 0x02080000),
        # Frame 5 (70% brightness)
        (0x0C000000, 0x08080000, 0x08000800, 0x00000800, 0x00040600, 0x00080000, 0x02080000),
        (0x0C000000, 0x07070000, 0x07000700, 0x00000700, 0x00030500, 0x00070000, 0x02070000),
        (0x0C000000, 0x07070000, 0x07000700, 0x00000700, 0x00040500, 0x00070000, 0x02070000),
        (0x0C000000, 0x06060000, 0x06000600, 0x00000600, 0x00030400, 0x00060000, 0x02060000),
        (0x0C000000, 0x06060000, 0x06000600, 0x00000600, 0x00030400, 0x00060000, 0x02060000),
        # Frame 10 (45% brightness)
        (0x0C000000, 0x05050000, 0x05000500, 0x00000500, 0x00030400, 0x00050000, 0x01050000),
        (0x0C000000, 0x04040000, 0x04000400, 0x00000400, 0x00020300, 0x00040000, 0x01040000),
        (0x0C000000, 0x04040000, 0x04000400, 0x00000400, 0x00020300, 0x00040000, 0x01040000),
        (0x0C000000, 0x03030000, 0x03000300, 0x00000300, 0x00010200, 0x00030000, 0x01030000),
        (0x0C000000, 0x03030000, 0x03000300, 0x00000300, 0x00020200, 0x00030000, 0x01030000),
        # Frame 15 (20% brightness)
        (0x0C000000, 0x02020000, 0x02000200, 0x00000200, 0x00010100, 0x00020000, 0x00020000),
        (0x0C000000, 0x01010000, 0x01000100, 0x00000100, 0x00010100, 0x00010000, 0x00010000),
        (0x0C000000, 0x01010000, 0x01000100, 0x00000100, 0x00000100, 0x00010000, 0x00010000),
        (0x0C000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000),
        # Frame 19 (0% brightness, only center green is lit)
        (0x0C000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000))

    def __init__(self, director_sm_id, projector_sm_id, pin_num):
        """
        Initializes the animation engine. We're taking all of the frame data and
        stuffing it into arrays for easy access. Then we rev up the neopixel director
        and projector state machines, start everything at frame zero, and we're ready.
        Once we're set up, the CPU will need to do very little, and only when a new
        frame is called for by the state machines.
        """
        print("  Neopixel Boot Animation object created.")
        # Calculate delay values (Calculated by desired frame rate and director frequency)
        self.SPIRAL_DELAY_VALUE = int(self.DIRECTOR_FREQ / self.SPIRAL_FPS) - 6
        self.PULSE_DELAY_VALUE = int(self.DIRECTOR_FREQ / self.PULSE_FPS) - 6
        self.FADE_DELAY_VALUE = int(self.DIRECTOR_FREQ / self.FADE_FPS) - 6
        # Prepare animation data (Package up frames in arrays for fast access)
        self.prepacked_spiral_frames = [array.array("I", frame) for frame in self.PRECOMPUTED_SPIRAL_FRAMES_DATA]
        self.prepacked_fade_frames = [array.array("I", frame) for frame in self.PRECOMPUTED_FADE_FRAMES_DATA]
        self.final_spiral_frame = self.prepacked_spiral_frames[-1]
        # Initialize State Machines
        self.sm_director = rp2.StateMachine(director_sm_id, neopixel_director, freq=self.DIRECTOR_FREQ)
        self.sm_projector = rp2.StateMachine(projector_sm_id, neopixel_projector, freq=self.PROJECTOR_FREQ, sideset_base=Pin(pin_num))
        # Initialize internal state variables
        self.animation_phase = 'SPIRAL'
        self.frame_counter = 0
        self.pulse_brightness = 0
        self.pulse_direction = self.PULSE_STEP

    def start(self):
        """Activates the state machines and starts the animation."""
        self.sm_director.active(1)
        self.sm_projector.active(1)
        # Send initial delay value to the director
        self.sm_director.put(self.SPIRAL_DELAY_VALUE)

    def stop(self):
        """Deactivates the state machines and clears the display."""
        self.sm_director.active(0) # Turn off the director
        blank_frame = array.array("I", [0] * self.NUM_PIXELS) # Assemble a blank (all 0) frame
        self.sm_projector.put(blank_frame)
        time.sleep(0.01) # Allow time for the blank frame to be sent
        self.sm_projector.active(0) # Turn off Projector

    @property
    def is_playing(self):
        """A property to check if the animation is still running."""
        return self.animation_phase != 'DONE'

    def update(self):
        """
        Paced by the neopixel_director state machine, this method checks what
        animation frame should be played next, grabs or creates the data, and 
        sends that data to the neopixel_projector. If needed, it can send a signal
        to the neopixel director to change frame timing.
        [Future expansion note: The director could be programmed to send back state info.]
        """
        if self.sm_director.rx_fifo():  # We'll only do this if there's something in the fifo
            self.sm_director.get() # Consume the data. Signal received!
            # Update is entirely based on animation phase, starting with SPIRAL.
            if self.animation_phase == 'SPIRAL':
                # Spiral is a slow seven-frame animation. Data for each frame is prepped in advance.
                self.sm_projector.put(self.prepacked_spiral_frames[self.frame_counter])
                self.frame_counter += 1
                if self.frame_counter >= len(self.prepacked_spiral_frames): # Spiral anim done
                    self.animation_phase = 'PULSE'
                    self.frame_counter = 0
                    self.sm_director.put(self.PULSE_DELAY_VALUE) # Change framerate
            elif self.animation_phase == 'PULSE':
                # Pulse is a little fancier. It takes a pre-packed frame of just the color
                # pixels, and then it calculates a value for the white channel.
                # It creates a new frame data array, and a fast "or" operation to merge color+white.
                self.pulse_brightness += self.pulse_direction
                white_mask = self.pulse_brightness
                pulse_frame = array.array("I", [0] * self.NUM_PIXELS) # Create a new frame
                pulse_frame[0] = self.final_spiral_frame[0] # Start with the filled-in spiral colors
                for i in range(1, self.NUM_PIXELS):
                    # Clever trick: Using "OR" to merge two binary streams of data.
                    # Here, we're merging the white channel into the RGB color data for each pixel.
                    pulse_frame[i] = self.final_spiral_frame[i] | white_mask
                self.sm_projector.put(pulse_frame) # Put it into the projector
                # Reverse the pulse direction once we hit the max brightness
                if self.pulse_brightness >= self.PULSE_MAX_BRIGHTNESS:
                    self.pulse_direction = -self.PULSE_STEP
                elif self.pulse_brightness <= 0: # Once we've reversed back to zero, the animation ends!
                    self.animation_phase = 'FADE'
                    self.pulse_brightness = 0
                    self.sm_director.put(self.FADE_DELAY_VALUE) # Set the new framerate
            elif self.animation_phase == 'FADE':
                # Like pulse, fade is a short animation with all the frames pre-prepped.
                self.sm_projector.put(self.prepacked_fade_frames[self.frame_counter])
                self.frame_counter += 1
                if self.frame_counter >= len(self.prepacked_fade_frames):
                    self.animation_phase = 'DONE'
        # This doesn't need to return anything, but we'll return the name of the animation
        # phase that we're on in case someone wants to look.
        return str(self.animation_phase)
    
### Initialization: NeoPixel Jewel Array ###
# We use state machines 5 and 6 on PIO1
neopixel_boot_anim = NeoPixelBootAnimation(director_sm_id=5, projector_sm_id=6, pin_num=NEOPIXEL_PIN)

### BEGIN BOOT SEQUENCE ###
try:
    print("Starting ...")
    neopixel_boot_anim.start()

    ### THE TEST IS UNDERWAY!! ###
    print("=========== TEST BEGINS ===========")
    time.sleep(4)
    neopixel_boot_anim.update()
        #Todo: Update the Neopixel once per frame or so. Use async or micropython.scheduler.
    print("============ TEST ENDS ============")

finally:
    print ("Test is finished or has been aborted.")
    ### END BOOT SEQUENCE AND CLEAN UP ###
    neopixel_boot_anim.stop()
    # Wait a moment before shutting down the channels
    time.sleep(0.25)
    print("Sequence Test Complete.")