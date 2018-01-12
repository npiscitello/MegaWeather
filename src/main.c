#include "ets_sys.h"
#include "osapi.h"
#include "driver/spi.h"

// transmit a register/data pair
#define spi_transmit(ADDR, DATA) spi_transaction(HSPI, 0, 0, 8, ADDR, 8, DATA, 0, 0);

// blink timers
static volatile os_timer_t on_timer;
static volatile os_timer_t off_timer;
#define PERIOD 2000
#define ON_TIME 1000

void ICACHE_FLASH_ATTR led_off(void *arg) {
  (void)arg;
  spi_transmit(0x0F, 0x00);
  os_timer_disarm(&off_timer);
}

void ICACHE_FLASH_ATTR led_on(void *arg) {
  (void)arg;
  spi_transmit(0x0F, 0x01);
  os_timer_arm(&off_timer, ON_TIME, 1);
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
  // take the chip out of shutdown
  spi_transmit(0x0C, 0x01);
  // turn off all pixels
  for( uint8_t i = 0x01; i < 0x08; i++ ) {
    spi_transmit(i, 0x00);
  }

  // setup timers
  os_timer_setfn(&on_timer, (os_timer_func_t *)led_on, NULL);
  os_timer_setfn(&off_timer, (os_timer_func_t *)led_off, NULL);
  os_timer_arm(&on_timer, PERIOD, 1);

}
