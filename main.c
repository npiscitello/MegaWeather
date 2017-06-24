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

// weather icons as noted - some stolen shamelessly from
// https://electricimp.com/docs/learning/weather/
// YCM will complain about this - it uses a GCC feature not implemented in clang
const __flash uint64_t icons[] =
{
  0x3c4299bdbd99423c,   // clear (sun)
  0x30180c0e0e0c1830,   // clear (moon)
  0x0000007e8181621c,   // partly cloudy
  0x0000007effff7e1c,   // cloudy
  0x8452087effff7e1c,   // precip (rain, sleet, etc.)
  0xa524e71818e724a5,   // snow
  0x7e01e61060fc020c,   // wind
  0xaa55aa55aa55aa55,   // fog
};

// YCM will complain about this - it uses a GCC feature not implemented in clang
const __flash uint64_t blank_screen = 0x0000000000000000;

// direct links to specific icons (IDK if they'll be useful, but might as well provide them)
#define SUN       0
#define MOON      1
#define P_CLOUD   2
#define CLOUD     3
#define PRECIP    4
#define SNOW      5
#define WIND      6
#define FOG       7
// Link to the first and last icons
#define FIRST     SUN
#define LAST      FOG

// update a register on the MAX72XX
void transmit(const uint8_t reg, const uint8_t val) {
  // set CS low
  PORTB &= 0xFF & !_BV(PORTB2);
  // what register to write on the MAX7221
  SPDR = reg;
  // wait for the transmission to finish
  while( !(SPSR & _BV(SPIF)) ) {}
  // what to write into the register on the MAX7221
  SPDR = val;
  // wait for the transmission to finish
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
 *  space:    number of blank columns between icons */
void slide_transition(const uint64_t out_icon, const uint64_t in_icon, const uint8_t space) {
  // index across frames
  for( uint8_t i = 1; i <= 8 + space; i++ ) {
    // modified update_screen - treat the icon like an 8-member uint8_t array
    // the shifts are 'backwards'; this is due to the writes being from bottom to top
    for( uint8_t j = 0x01; j < 0x09; j++ ) {
      transmit(j, ((((uint8_t*)&out_icon)[j - 1]) >> i) | ((((uint8_t*)&in_icon)[j - 1]) << (8 - i + space)));
    }
    _delay_ms(150);
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
  slide_transition(blank_screen, icons[FIRST], 0);
  _delay_ms(1000);

  // roll through the defined icons
  while( 1 == 1 ) {
    for( uint8_t i = FIRST; i <= LAST; i++ ) {
      if( i != LAST ) {
        slide_transition(icons[i], icons[i+1], 2);
      } else {
        slide_transition(icons[LAST], icons[FIRST], 2);
      }
      _delay_ms(1000);
    }
  }
}
