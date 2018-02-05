#include "os_type.h"
#include "osapi.h"
#include "driver/spi_interface.h"

#include "graphics.h"

#include "display.h"

/* execute a single transition
 * trans: a struct defining the transition
 */
void update_screen( const icon_t image );
void ICACHE_FLASH_ATTR execute_transition() {
  gpio_output_set(BIT5, 0, 0, 0);
  update_screen(image_arr[SNOW]);
  return;
}



// convenience function to transmit SPI
// yeah, it has to allocate memory for an SpiData struct every time, but... premature optimization!
// <TODO> use callback to implement mutex?
void ICACHE_FLASH_ATTR spi_transmit( const uint8_t addr, const uint8_t data ) {
  SpiData spistruct;
  spistruct.cmd = addr;
  spistruct.cmdLen = 1;
  spistruct.addr = 0;
  spistruct.addrLen = 0;
  spistruct.data = (uint32_t*)&data;
  spistruct.dataLen = 1;

  SPIMasterSendData(SpiNum_HSPI, &spistruct);
  return;
}



// set up the screen and other variables for first use
void ICACHE_FLASH_ATTR display_init() {

  // init external SPI pins
  WRITE_PERI_REG(PERIPHS_IO_MUX, 0x105);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, 2);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 2);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, 2);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, 2);

  // use the external (not-flash) pins with the specified settings
  SpiAttr spi;
    spi.mode = SpiMode_Master;
    // clock inactive low (CPOL 0), data on leading edge (CPHA 0)
    spi.subMode = SpiSubMode_0;
    spi.speed = SpiSpeed_10MHz;
    spi.bitOrder = SpiBitOrder_MSBFirst;
  SPIInit(SpiNum_HSPI, &spi);

  // setup for the MAX7221 chip (through a TXB0104 level shifter)
  // don't use the decode table
  spi_transmit(0x09, 0x00);
  // set intensity to middle ground
  spi_transmit(0x0A, 0x08);
  // scan across all digits
  spi_transmit(0x0b, 0x07);
  // turn off all pixels - I could use update_screen for this, but I don't need the fancy shifting
  // I'll probably start using it if I need to worry about mutexes
  for( uint8_t i = 0x01; i <= 0x08; i++ ) {
    spi_transmit(i, 0x00);
  }
  // take the chip out of shutdown
  spi_transmit(0x0C, 0x01);
  return;
}



// update the whole screen in one shot
// I'm not worrying about mutex locking...yet. I'll figure something out if it becomes a problem.
void ICACHE_FLASH_ATTR update_screen( const icon_t image ) {
  gpio_output_set(0, BIT5, 0, 0);
  for( uint8_t i = 0x01; i <= 0x08; i++ ) {
    // this could probably be abstracted away a bit; for now, we know we're using uint64_t to
    // simulate an 8 member uint8_t array, so we'll keep this magic number for now...
    spi_transmit(i, (uint8_t)(image.icon >> ((i - 1) * 8)));
  }
  return;
}
