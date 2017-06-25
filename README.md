# MegaWeather

A simple ATMEGA328P based weather station to display current and future weather status.

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

I use a USBtinyISP (https://www.adafruit.com/products/46) to program the chip; any other ISP should
work (as long as the avrdude command in the Makefile is updated to reflect the change). `make 
program` should build `main.hex` with intermediate files and send it to the chip. If you have the
ATMEGA328 in your avrdude.conf, this will also work with that chip - just modify the `avrdude`
command in the Makefile accordingly.

## Notes

* See [xantorohara's project](https://github.com/xantorohara/led-matrix-editor) for a really cool
  browser-based tool for 8x8 matrix planning. Use the following address to access the
  tool with the weather icons pre-loaded!

      http://xantorohara.github.io/led-matrix-editor/#3c4299bdbd99423c|30180c0e0e0c1830|0000007e8181621c|0000007effff7e1c|8452087effff7e1c|a524e71818e724a5|7e01e61060fc020c|aa55aa55aa55aa55

* This will probably not be ATMEGA328P based forever; I'm looking at the ESP8266 as a (tiny!)
  single-chip solution
