#include "ets_sys.h"
#include "osapi.h"
#include "driver/spi.h"
#include "graphics.h"

// transmit a register/data pair
#define spi_transmit(ADDR, DATA) spi_transaction(HSPI, 0, 0, 8, ADDR, 8, DATA, 0, 0);

// blink timers
static volatile os_timer_t trans_timer;
#define PERIOD 2000

uint8_t counter = 0x00;

void ICACHE_FLASH_ATTR disp_image(void *arg) {
  (void)arg;
  for( uint8_t i = 0x01; i <= 0x08; i++ ) {
    spi_transmit(i, (uint8_t)(icon[counter] >> ((i - 1) * 8)));
  }
  
  if( counter++ == LAST ) {
    counter = FIRST;
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
  os_timer_setfn(&trans_timer, (os_timer_func_t *)disp_image, NULL);
  os_timer_arm(&trans_timer, PERIOD, 1);

}
