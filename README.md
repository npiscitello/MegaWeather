# Project Planning

~~This is a to-do list of sorts in preparation for the hackathon. This project has officially moved to
the esp8266 platform.~~ This list was to help plan for work during a hackathon; now, it's just the
general project todo list.

0. ~~get some kind of debugging output console over usb?~~
1. ~~SPI comms (get *something* to display on the matrix)~~
2. ~~re-implement icon set~~
3. ~~animations!~~
4. Get Wifi connected and pull dummy data from somewhere (try example.com)
5. ~~find a weather API (to hit every hour or half hour ish)~~
6. sync icons with API; add as needed - get them displaying properly!
7. display temperature, precip % chance, etc.

# MegaWeather

A simple ESP8266 based weather station to display current and future weather status.

## Features

### Current:

* Full set of static weather icons, numerals, and useful characters
* Icon and number slide transition animation (non-blocking!)

### Planned:

* Network connectivity to pull live weather updates
* battery-powered for maximum flexibility
* current weather / forecast / both display selector

### Proposed:

* fancy animated weather icons
* indoor temperature
* adjustable / auto brightness

## Build:

This project has officially moved to the ESP8266 platform (specifically the dev breakout board from
SparkFun).

AVR details:
I use a USBtinyISP (https://www.adafruit.com/products/46) to program the chip; any other ISP should
work (as long as the avrdude command in the Makefile is updated to reflect the change). `make 
program` should build `main.hex` with intermediate files and send it to the chip. If you have the
ATMEGA328 in your avrdude.conf, this will also work with that chip - just modify the `avrdude`
command in the Makefile accordingly.

## Notes

* The ESP8266 SPI library is **not mine**! It was researched and built by David Ogilvy (aka 
  MetalPhreak), who did a *very* nice job with it! Check inside the lib folder for more
  information, including a link to his blog!

* See [xantorohara's project](https://github.com/xantorohara/led-matrix-editor) for a really cool
  browser-based tool for 8x8 matrix planning. Use the following address to access the
  tool with the weather icons pre-loaded!

      http://xantorohara.github.io/led-matrix-editor/#3c4299bdbd99423c|30180c0e0e0c1830|0000007e8181621c|0000007ed5ab761c|0000007effff7e1c|8452087effff7e1c|a524e71818e724a5|7e01e61060fc020c|aa55aa55aa55aa55

* ~~This will probably not be ATMEGA328P based forever; I'm looking at the ESP8266 as a (tiny!)
  single-chip solution~~ See above; the platform changeover is done.

* Pin mapping: since the LED cathodes in the TC15-11EWA that I'm using are on the columns, and the
  MAX72XX chips are common cathode drivers, we write the display in columns instead of the more
  usual rows (AKA, we can specify the state of the 8 individual LEDs in a single column with one
  write). The matrix generator mentioned above generates row oriented data. By reversing the order of
  the rows and turning the matrix 90 degrees, we can make it work like a row oriented display.
    * R1 = DP, R2 = A, R3 = B, R4 = C, R5 = D, R6 = E, R7 = F, R8 = G
    * C1 = D0, C2 = D1, C3 = D2, C4 = D3, C5 = D4, C6 = D5, C7 = D6, C8 = D7

* Looking for simple example code for the ESP8266? Check out my [other
  repo](https://github.com/npiscitello/esp8266_resources).

* TODO:
  * Should users be able to add to the queue while its executing? Should they be able to clear it?
    Clearing the queue during execution serves effectively as cancelling the execution...
