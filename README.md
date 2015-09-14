# TeensyDump

A setup to use two Teensy 3.1 microprocessors to dump an Intel 28F400 NOR FLASH
chip.

## Why?

Always with the why? Because my ancient Dataman S4 doesn't support the 44-pin
PSOP package, but I happen to have a socket that fits in a breadboard, so...

### Why _TWO_ Teensys?

Oh, right. A single Teensy does have enough digital I/O, but only if you solder
to the pads on the back. Not the most breadboard friendly.

### Isn't this slow?

Not as slow as you'd think. I set the Teensys to use 6,000,000 baud for serial,
which works pretty well. You do have to overclock them to 96 MHz for that to
work right.

## Schematics

I'll get to it, probably. Honestly, just read the pin definitions in the source
code, and follow the datasheet for the 28F400. It is pretty straightforward.
