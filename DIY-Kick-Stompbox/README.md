# DIY Kick Stompbox - program for the Arduino Nano

by **3Dvarius**. A battery-powered kick drum pedal: stomp it and it plays a kick through a guitar cable.
The 3D model and the print profile are on MakerWorld; this repository has the program, the sounds,
the sample converter, the wiring diagram and the build guide.

## Download

- **Everything in one file:** [DIY_Kick_Stompbox_firmware.zip](DIY_Kick_Stompbox_firmware.zip)
  (click it, then the download button on the right)
- **Build guide (PDF):** [DIY_Kick_Stompbox_Build_Guide.pdf](DIY_Kick_Stompbox_Build_Guide.pdf)
- **Sample converter online:** open the link in the "About" box of this page (GitHub Pages),
  or open `sample_converter.html` from the zip with a double-click: it also works offline.

## What's inside

```
stompbox_kick/
  stompbox_kick.ino       the program: open this in the Arduino IDE
  sample_a.h              sound A: acoustic kick
  sample_b.h              sound B: 808 kick
sample_converter.html     makes your own sound files (opens in your browser)
spare_sounds/             copies of the two supplied sounds
wiring_diagram.png
```

## Sounds and knob

The program looks at the sound files in its folder and sets the knob by itself:

| Files in `stompbox_kick` | The knob |
|--------------------------|----------|
| `sample_a.h` + `sample_b.h` (as supplied) | blends: fully left = sound A, fully right = sound B |
| only one of the two | volume of that sound (fully left = silent) |

So with the supplied files the knob blends the acoustic kick with the 808.
Delete one of the two files and the knob becomes the volume of the other one.

## Using your own sounds

1. Open the online converter (link in "About"), or `sample_converter.html` in your browser (double-click it: it works
   offline on Windows and Mac, nothing is uploaded).
2. Drop in an audio file with one hit (WAV or MP3), choose Sound A or Sound B,
   listen, and save `sample_a.h` or `sample_b.h`.
3. Put the file in the `stompbox_kick` folder, next to `stompbox_kick.ino`
   (replace the old one if asked).
4. If the sketch is open in the Arduino IDE, close it and open it again,
   then upload.

Each sound can be up to 0.68 s long. To get a supplied sound back, copy it
from `spare_sounds`.

## Loading the program

1. Install the free Arduino IDE (arduino.cc).
2. Keep the folder name `stompbox_kick`, with the files inside.
3. Open `stompbox_kick.ino`.
4. Tools → Board → **Arduino Nano**, Processor → **ATmega328P**
   (if the upload fails, choose **ATmega328P (Old Bootloader)**).
5. Connect the USB cable, choose the Port, press Upload.

## Sensitivity

The first setting in the program, inside the big box of `#` signs:

```
#define SENSITIVITY 5
```

1 = only hard stomps play ... 10 = light taps play too.
It plays by itself → lower the number. Light stomps don't play → raise it.
Then upload again.

## The LED

| LED        | Meaning                                           |
|------------|---------------------------------------------------|
| on         | pedal on, battery OK (or powered from USB)        |
| slow blink | battery low (under 7 V): change it soon           |
| fast blink | battery almost empty (under 6.5 V): change it now |

## Tests (before closing the pedal)

Tools → Serial Monitor, 115200 baud. Type a number and press Enter.
The first lines also tell you which sounds the pedal is using.

| Type | Test |
|------|------|
| 3 | jack: 2-second beep on the output |
| 1 | piezo: numbers stay low at rest, go up when you stomp |
| 2 | knob: goes from 0 to 1023 end to end |
| 4 / 5 | plays sound A / sound B (with one sound: 4 plays it) |
| 6 | battery: switch the pedal ON, a new battery reads 9.0-9.6 V |
| 0 | stop |

## Licence

© 2026 3Dvarius. Program, sounds, sample converter and wiring diagram:
CC BY-NC-ND 4.0 - see LICENSE. You may change the settings and the sounds for
your own pedal; you may not publish modified versions or sell it.
The 3D model is published separately under the Standard Digital File License
(personal use only).
