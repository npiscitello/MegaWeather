#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "driver/uart.h"
#include "driver/spi.h"

// UART baud rate
#define BAUD BIT_RATE_115200

// pin the LED is on
#define GPIO BIT5

// the LED is on for 50ms every 1000ms
#define LEDPER 1000
#define LEDON 50

// the matrix will change every half a second
#define SPIPER 500

static volatile os_timer_t on_timer;
static volatile os_timer_t off_timer;
static volatile os_timer_t spi_timer;

uint8_t spi_state;

void ICACHE_FLASH_ATTR write_reg( uint8_t reg, uint8_t val ) {
  spi_transaction(HSPI, 0, 0, 8, reg, 8, val, 0, 0);
}

void ICACHE_FLASH_ATTR led_off( void *arg ) {
  gpio_output_set(GPIO, 0, 0, 0);
  os_timer_disarm(&off_timer);
}

void ICACHE_FLASH_ATTR led_on( void *arg ) {
  gpio_output_set(0, GPIO, 0, 0);
  os_timer_arm(&off_timer, LEDON, 1);
}

void ICACHE_FLASH_ATTR spi_switch( void *arg ) {
  spi_state = ~spi_state;
  for( uint8_t i = 0x01; i <= 0x08; i++ ) {
    write_reg(i, spi_state);
  }
}

void ICACHE_FLASH_ATTR user_init() {
  // init gpio subsytem
  gpio_init();

  // config GPIO5 to actually be a BPIO and enable output
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
  gpio_output_set(0, 0, GPIO, 0);
  gpio_output_set(GPIO, 0, 0, 0);

  // setup LED timers
  os_timer_setfn(&on_timer, (os_timer_func_t *)led_on, NULL);
  os_timer_setfn(&off_timer, (os_timer_func_t *)led_off, NULL);
  os_timer_arm(&on_timer, LEDPER, 1);

  // setup SPI
  spi_init(HSPI);

  // don't use the docde table for any digit
  write_reg(0x09, 0x00);

  // set intensity to middle ground
  write_reg(0x0A, 0x08);

  // scan across all digits
  write_reg(0x0B, 0x07);

  // clear display
  spi_state = 0x00;
  for( uint8_t i = 0x01; i <= 0x08; i++ ) {
    write_reg(i, spi_state);
  }

  // set up SPI timer
  os_timer_setfn(&spi_timer, (os_timer_func_t *)spi_switch, NULL);
  os_timer_arm(&spi_timer, SPIPER, 2);
}
