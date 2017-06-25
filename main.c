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
// allow constants to be stored in flash instead of SRAM
#include <avr/pgmspace.h>
// icon sets
#include "graphics.h"

// data for slide animation
struct animation_data {
  uint64_t  out_icon;       // icon to be replaced
  uint64_t  in_icon[3];     // new icons to be shown (in_icon[0] used if icon_slide == true)
  uint8_t   number;         // number reflected in in_icon; hopefully, I can find a way to not need this
  uint8_t   frame;          // current frame of the animation
  uint8_t   space       :3; // space between icons; for more than 7 columns, use character[BLANK]
  uint8_t   icon_slide  :1; // true if transitioning between 2 icons, false if displaying a number
  uint8_t   slide_en    :1; // slide enable flag
  uint8_t   slide_done  :1; // slide finished flag
  uint8_t   bool6       :1;
  uint8_t   bool7       :1;
} adata;
// these aren't defined
#define false 0
#define true  1

// how long to delay between frames in a slide transition, in ms (0-255)
#define SLIDE_DELAY 100

// update a register on the MAX72XX
void transmit(const uint8_t reg, const uint8_t val) {
  // set CS low
  PORTB &= 0xFF & !_BV(PORTB2);
  // what register to write on the MAX7221
  SPDR = reg;
  // wait for the transmission to finish (poll the finish flag)
  // this could be interrupt based but that would be needlessly
  // complicated and actually slower!
  while( !(SPSR & _BV(SPIF)) ) {}
  // what to write into the register on the MAX7221
  SPDR = val;
  // wait for the transmission to finish (poll the finish flag)
  while( !(SPSR & _BV(SPIF)) ) {}
  // bring CS high
  PORTB |= 0x00 | _BV(PORTB2);
  return;
}

// update the entire matrix from a 64-bit number
void update_screen(const uint64_t icon) {
  // transmit 8 rows at a time. We're taking advantage of the fact that 
  // the display registers start at 1 on the MAX72XX
  // treat the icon like a uint8_t array
  for( uint8_t i = 0x01; i < 0x09; i++ ) {
    transmit(i, ((uint8_t*)&icon)[i - 1]);
  }
  return;
}

/* slide transition between icons
 *  out_icon: old icon to be replaced
 *  in_icon:  new icon to be displayed
 *  space:    number of blank columns between icons
 *
 * I'm re-implementing this as non-blocking; each call turns into almost 3 seconds of execution,
 * most of that time being _delay_ms(). */
void icon_slide_transition(const uint64_t out_icon, const uint64_t in_icon, const uint8_t space) {
  // index across frames
  for( uint8_t i = 1; i <= space + 8; i++ ) {
    // modified update_screen - treat the icon like an 8-member uint8_t array
    // the shifts are 'backwards'; this is due to the writes being from bottom to top
    for( uint8_t j = 0x01; j < 0x09; j++ ) {
      transmit(j, ((((uint8_t*)&out_icon)[j - 1]) >> i) | ((((uint8_t*)&in_icon)[j - 1]) << (space + 8 - i)));
    }
    _delay_ms(SLIDE_DELAY);
  }
  return;
}

/* slide transition to show a number between icons. It will stop on the last number and return the 
 * index of that digit; this can be used to transition from that digit back to an icon.
 *  out_icon: icon to be replaced
 *  number:   number to be displayed. This is passed as an integer that will be parsed.
 *  space:    number of blank columns between icon and number
 *
 * I'm re-implementing this as non-blocking; each call turns into almost 3 seconds of execution,
 * most of that time being _delay_ms(). */
// <TODO> find a way to add a degrees sign? Is this even necessary?
uint8_t number_slide_transition(const uint64_t out_icon, const uint8_t number, const uint8_t space) {
  // parse digits in the number into a temp working array depending on number of digits
  uint64_t number_digs[3];
  if( number > 99 ) {
    number_digs[0] = digit[number / 100];
    number_digs[1] = digit[(number / 10) % 10];
    number_digs[2] = digit[number % 10];
  } else if ( number > 9 ) {
    number_digs[0] = digit[(number / 10) % 10];
    number_digs[1] = digit[number % 10];
    number_digs[2] = character[BLANK];
  } else {
    number_digs[0] = digit[number % 10];
    number_digs[1] = character[BLANK];
  }

  /* we need three for loops because, with just one, the arguments to the left shifts would become
   * negative. This could probably be mitigated with some macro magic, but I am no magician.
   * Besides, I find this way clearer. */

  // wipe across the first space + 8 columns (clear icon)
  uint8_t i, last_i;
  for( i = 1; i <= space + 8; i++ ) {
    for( uint8_t j = 0x01; j < 0x09; j++ ) {
      transmit(j, ((((uint8_t*)&out_icon)[j - 1]) >> i) | 
          ((((uint8_t*)&number_digs[0])[j - 1]) << (space + 8 - i)) |
          ((((uint8_t*)&number_digs[1])[j - 1]) << (space + 8 + 6 - i)));
    }
    _delay_ms(SLIDE_DELAY);
  }
  last_i = i;

  if( number > 9 ) {
    // wipe across the next 6 columns (clear first digit)
    for( i++; i <= last_i + 6; i++ ) {
      for( uint8_t j = 0x01; j < 0x09; j++ ) {
        transmit(j, ((((uint8_t*)&number_digs[0])[j - 1]) >> (i - last_i)) | 
            ((((uint8_t*)&number_digs[1])[j - 1]) << (last_i + 6 - i)) |
            ((((uint8_t*)&number_digs[2])[j - 1]) << (last_i + 6 + 6 - i)));
      }
      _delay_ms(SLIDE_DELAY);
    }
    last_i = i;
  }

  if( number > 99 ) {
    // wipe across the final 6 columns (clear second digit)
    for( i++; i <= last_i + 6; i++ ) {
      for( uint8_t j = 0x01; j < 0x09; j++ ) {
        transmit(j, ((((uint8_t*)&number_digs[1])[j - 1]) >> (i - last_i)) | 
            ((((uint8_t*)&number_digs[2])[j - 1]) << (last_i + 6 - i)));
      }
      _delay_ms(SLIDE_DELAY);
    }
  }

  return number % 10;
}

/* slide transition to show a number between icons. It will stop on the last number and return the 
 * index of that digit; this can be used to transition from that digit back to an icon.
 *  out_icon: icon to be replaced
 *  number:   number to be displayed. This is passed as an integer that will be parsed.
 *  space:    number of blank columns between icon and number (0-7)
 * 
 * this is the nonblocking version of the above function */
// <TODO> degree sign? Is this necessary?
uint8_t number_slide_transition_async(const uint64_t out_icon, const uint8_t number, const uint8_t space) {
  adata.icon_slide = false;
  adata.slide_en = true;
  adata.out_icon = out_icon;
  adata.space = space;
  adata.number = number;
  // parse digits in the number into the digit frame array
  if( number > 99 ) {
    adata.in_icon[0] = digit[number / 100];
    adata.in_icon[1] = digit[(number / 10) % 10];
    adata.in_icon[2] = digit[number % 10];
  } else if ( number > 9 ) {
    adata.in_icon[0] = digit[(number / 10) % 10];
    adata.in_icon[1] = digit[number % 10];
    adata.in_icon[2] = character[BLANK];
  } else {
    adata.in_icon[0] = digit[number % 10];
    adata.in_icon[1] = character[BLANK];
  }
  // setup and enable timer - /1024 prescaler on 1MHz gives about 1ms/tick
  TCCR0A = _BV(WGM01);
  TCCR0B = _BV(CS02) | _BV(CS00);
  OCR0A = SLIDE_DELAY;

  // set the timer value to zero before enabling the interrupt to ensure we wait the full period
  // before triggering the second frame.
  TCNT0 = 0x00;
  TIMSK0 = _BV(OCIE0A);
  return number % 10;
}

// this does all the work - it is to be called in a loop to generate the animation
void number_slide_transition_animation_loop(void) {
  /* we need three for loops because, with just one, the arguments to the left shifts would become
   * negative. This could probably be mitigated with some macro magic, but I am no magician.
   * Besides, I find this way clearer. */

  // wipe across the first space + 8 columns (clear icon)
  if( adata.frame <= adata.space + 8 ) {
    for( uint8_t j = 0x01; j < 0x09; j++ ) {
      transmit(j, ((((uint8_t*)&adata.out_icon)[j - 1]) >> adata.frame) | 
          ((((uint8_t*)&adata.in_icon[0])[j - 1]) << (adata.space + 8 - adata.frame)) |
          ((((uint8_t*)&adata.in_icon[1])[j - 1]) << (adata.space + 8 + 6 - adata.frame)));
    }
  } else if( adata.frame <= adata.space + 8 + 6
      && adata.frame > adata.space + 8
      && adata.number > 9) {
    // wipe across the next 6 columns (clear first digit)
    for( uint8_t j = 0x01; j < 0x09; j++ ) {
      transmit(j, ((((uint8_t*)&adata.in_icon[0])[j - 1]) >> adata.frame) | 
          ((((uint8_t*)&adata.in_icon[1])[j - 1]) << (adata.space + 8 + 6 - adata.frame)) |
          ((((uint8_t*)&adata.in_icon[2])[j - 1]) << (adata.space + 8 + 6 + 6 - adata.frame)));
    }
  } else if( adata.frame <= adata.space + 8 + 6 + 6
      && adata.frame > adata.space + 8 + 6
      && adata.number > 99 ) {
    // wipe across the final 6 columns (clear second digit)
    for( uint8_t j = 0x01; j < 0x09; j++ ) {
      transmit(j, ((((uint8_t*)&adata.in_icon[1])[j - 1]) >> (adata.frame)) | 
          ((((uint8_t*)&adata.in_icon[2])[j - 1]) << (adata.space + 8 + 6 + 6 - adata.frame)));
    }
  } else {
    // this means that all of the digits have scrolled
    adata.slide_done = true;
  }

  return;
}

int main(void) {
  // turn off everything except the SPI interface
  PRR = 0xFF & !_BV(PRSPI);

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

  // slide the first icon onto the screen
  icon_slide_transition(character[BLANK], icon[FIRST], 0);
  _delay_ms(1000);

  // roll through the defined icons
  uint8_t last_digit = 0;
  while( 1 == 1 ) {
    last_digit = number_slide_transition(icon[FIRST], 240, 3);
    icon_slide_transition(digit[last_digit], icon[FIRST], 2);
    _delay_ms(2000);
  }
}
