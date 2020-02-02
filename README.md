SMS Test Suite
==============

*largely inspired by Artemio's 240p, but not a fork of it!*

## Functions:

### Video Tests
(video test descriptions mainly taken from http://junkerhq.net/xrgb/index.php?title=240p_test_suite)

* PLUGE
(press 1/START or 2 to exit)
The picture line-up generation equipment (PLUGE) is a test pattern used to adjust the black level and contrast of a video signal.
The pattern in this suite is mainly designed to adjust the black level. The control for setting this on your TV is usualy named "Brightness". White level on CRT monitors can also be easily calibrated with it.

* Color bars
(press 1/START or 2 to exit)
This is a typical pattern for adjusting black, white, and color levels in general. Displaying all the "pure" colors possible on each system, from lowest to highest intensity.
This is adjusted in the same way black levels are set with the PLUGE. Adjust the white level first, this is done using the "Contrast" control on your TV set.

* Color bleed
(press 1/START to change the vertical bars to a checker board and back, press 2 to exit)
This pattern helps diagnose unneeded color up-sampling. It consists of one pixel width color bars alternating with one pixel width black bars.

* Grid
(press 1/START or 2 to exit)
It is used in order to align the screen and find out overscan on the display. It uses the full resolution of the target console, with a margin of red squares for the corners and an additional internal margin of yellow squares.
Consumer CRTs usually don't allow compensating for overscan, and many games don't draw outside the white/yellow square borders.

* Stripes/Checks
(press 1/START to change the bars to a checker board and back, press 2 to exit)
A pattern consisting of a full screen of horizontal black and white stripes, one pixel tall each. This is a taxing pattern
Checker board then is an even more taxing pattern, a one pixel black and one pixel white alternating pattern. This pattern also makes obvious if all lines are being scaled equally.

* Full colors
(press 1/START to cycle white/colors, press 2 to exit)
The screen is filled with solid color. The user can change the fill color cycling between white, red, green and blue.

* Linearity
(press 1/START or 2 to exit)
Five circles are displayed, one in each corner and a big one in the middle. Used on CRTs to check linearity of the display or scaler. The pattern is designed to work in either NTSC or PAL, respecting the pixel aspect ratios. Circles should be perfectly round in the display.
The linearity of the display or upscaler can be verified by measuring the diameter of the circles. Of course the linearity should be kept in all directions.
Should work on SEGA Game Gear too!

* Drop shadow
(press 1/START or 2 to exit)
This is a crucial test for 240p upscan converters. It displays a simple sprite shadow against a background, but the shadow is shown only on each other frame. On a CRT this achieves a flicker/transparency effect, since you are watching a 25Hz/30Hz shadow on a 50Hz/60Hz signal. No background detail should be lost and the shadow should be visible.
This is a very revealing test since it exposes how the device is processing the signal. On devices that can't handle 240p signals, you can usually see that the signal is being interlaced with an odd and an even frame, as if it were regular 480i. This shows a shadow that doesn't flicker, with feathering (a line is drawn and the next one isn't).

* Striped sprite
(press UP/LEFT or DOWN/RIGHT to move the sprite diagonally, press 1/START or 2 to exit)
There are actually deinterlacers out there which can display the drop shadows correctly and still interpreted 240p as 480i. With a striped sprite it should be easy to tell if a processor tries to deinterlace (plus interpolate) or not.

### Audio Tests

* press UP to play music on PSG (square wave) channel 0
* press RIGHT to play music on PSG (square wave) channel 1
* press DOWN to play music on PSG (square wave) channel 2
* press LEFT to play music on PSG noise channel (channel 3)
* press 1/START to test volume clipping. If the sound volume appears to do not change it means it's clipping, which is very common on a SEGA Master System, uncommon on a SEGA MegaDrive/Genesis.
* press 2 to exit

### Pad Tests

You can test your SEGA Master System and SEGA MegaDrive/Genesis (*port A only*) pads, your console PAUSE and RESET keys, and your console ports too. If no key is pressed/held/released in approx 3 seconds, the test will end.

### Paddle Tests

You can test your SEGA Master System (Japanese) Paddle(s) in either port. If the knob isn't rotated or paddle key is not pressed/held/released in approx 3 seconds the test will end. PAUSE will also end the test. If paddle wasn't connected when the Test Suite started, you can connect that while in the main menu and press PAUSE to restart it, to have that detected. If two paddles are connected when the Test Suite starts, paddle test will be run immediatelym but it won't be possible to leave the test.

### System Info

You can read some info about your system. In detail:

* The result of a detection routine that tests if your console is a Japanese one or an 'import' one (USA/Europe).
* The result of the VDP type detection routine: first revision SEGA Master System should have a 315-5124 chip while the newer SEGA Master System II should have a 315-5246 chip. Note that this is not always true.
* The result of the TV type detection routine: either 50Hz (PAL) or 60Hz (NTSC).
* The result of a detection routine that tests if your console is a SEGA Game Gear.
* The 16-bit checksum of the first 8 KB of the console BIOS contents (only if it's not a SEGA Game Gear). This is used to identify the BIOS in the machine, and informations will be printed in the lower part of the screen.
If "unidentified BIOS found!" is printed, please let us know here -> http://www.smspower.org/forums/
Also, pressing the DOWN key on the pad, the first 16 KB of the BIOS contents will be saved to SRAM, if your cartridge supports that (Krikzz's Master EverDrive's users will find a SAV file in their SD cards only after dumping the SRAM to SD, which happens when you load a new ROM to the cart).
