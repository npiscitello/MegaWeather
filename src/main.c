#include "ets_sys.h"
#include "osapi.h"
#include "graphics.h"
#include "animation.h"


// blink timers
static volatile os_timer_t change_timer;
#define PERIOD 500

uint8_t counter = 0x00;

void ICACHE_FLASH_ATTR disp_image(void *arg) {
  (void)arg;
  if( !transition_running() ) {

    if( counter <= 9 ) {
      transition(digit[counter], NUM_W, 1, 100);
    } else {
      transition(icon[FOG], 8, 1, 100);
    }
    
    if( counter++ == 10 ) {
      counter = FIRST_ICON;
    }
  }
}

void ICACHE_FLASH_ATTR user_init()
{
  // use the external (not-flash) pins
  spi_init(HSPI);

  // valid data on clock leading edge, clock is low when inactive
  spi_mode(HSPI, 0, 0);

  // setup for the MAX7221 chip (through a TXB0104 level shifter)
  // don't use the decode table
  spi_transmit(0x09, 0x00);
  // set intensity to middle ground
  spi_transmit(0x0A, 0x08);
  // scan across all digits
  spi_transmit(0x0b, 0x07);
  // turn off all pixels
  for( uint8_t i = 0x01; i <= 0x08; i++ ) {
    spi_transmit(i, 0x00);
  }
  // take the chip out of shutdown
  spi_transmit(0x0C, 0x01);

  // setup timers
  os_timer_setfn(&change_timer, (os_timer_func_t *)disp_image, NULL);
  os_timer_arm(&change_timer, PERIOD, 1);

}
