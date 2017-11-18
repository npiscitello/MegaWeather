/* Nick Piscitello
 * February 2017 (project start)
 * Atmel ATMEGA328P-PU
 * avr-gcc 6.3.0 (on Arch Linux) (project start)
 * fuses: default (l: 0x62, h: 0xD9, e: 0x07)
 * system clock: 8MHz / 8 = 1MHz */

// chip hardware defs, etc.
#include <avr/io.h>
// wait for a certain amount of time (for debugging and development)
#include <util/delay.h>
// icon sets
#include "graphics.h"
// animation functions
#include "animation.h"



// take a 1, 2, or 3 digit number and parse it into three digits to fill transition_data.in_icon
void num_to_icons(transition_data* tdata, uint8_t number) {
  tdata->number = number;
  if( number > 99 ) {
    tdata->in_icon[0] = digit[number / 100];
    tdata->in_icon[1] = digit[(number / 10) % 10];
    tdata->in_icon[2] = digit[number % 10];
  } else if ( number > 9 ) {
    tdata->in_icon[0] = digit[(number / 10) % 10];
    tdata->in_icon[1] = digit[number % 10];
    tdata->in_icon[2] = character[BLANK];
  } else {
    tdata->in_icon[0] = digit[number % 10];
    tdata->in_icon[1] = character[BLANK];
  }
}



int main(void) {
  // turn off everything except the SPI interface and timer0
  PRR = 0xFF & !_BV(PRSPI) & !_BV(PRTIM0);

  // set up SPI: master, CLK = system clock / 2
  // pins: SS = PB2, MOSI = PB3, SCK = PB5
  DDRB = _BV(DDB2) | _BV(DDB3) | _BV(DDB5);
  SPCR = _BV(SPE) | _BV(MSTR);
  SPSR = _BV(SPI2X);

  // set up MAX7219
  // don't use the decode table for any digit
  transmit(0x09, 0x00);
  // set intensity to middle ground
  transmit(0x0A, 0x08);
  // scan across all digits
  transmit(0x0B, 0x07);
  // clear display
  for( uint8_t i = 0x01; i <= 0x08; i++ ) {
    transmit(i, 0x00);
  }
  // take the chip out of shutdown mode
  transmit(0x0C, 0x01);

  // set up transition timer - disable clock until we need it
  TCCR0A |= _BV(WGM01);
  TCCR0B &= (0xFF & !_BV(CS02) & !_BV(CS00));
  OCR0A = SLIDE_DELAY;
  
  // define a transition data storage struct
  transition_data tdata;

  // slide the first icon onto the screen
  icon_transition(&tdata, character[BLANK], icon[FIRST], 0);
  wait_for_transition(&tdata);
  _delay_ms(2000);

  uint8_t last_digit = 0;
  while( 1 == 1 ) {
    // three digit number
    num_to_icons(&tdata, 240);
    last_digit = number_transition(&tdata, icon[FIRST], 3);
    wait_for_transition(&tdata);
    icon_transition(&tdata, digit[last_digit], icon[PRECIP], 2);
    wait_for_transition(&tdata);
    _delay_ms(2000);

    // two digit number
    num_to_icons(&tdata, 36);
    last_digit = number_transition(&tdata, icon[PRECIP], 3);
    wait_for_transition(&tdata);
    icon_transition(&tdata, digit[last_digit], icon[SNOW], 2);
    wait_for_transition(&tdata);
    _delay_ms(2000);

    // one digit number
    num_to_icons(&tdata, 9);
    last_digit = number_transition(&tdata, icon[SNOW], 3);
    wait_for_transition(&tdata);
    icon_transition(&tdata, digit[last_digit], icon[FIRST], 2);
    wait_for_transition(&tdata);
    _delay_ms(2000);
  }
}