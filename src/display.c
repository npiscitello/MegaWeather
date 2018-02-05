#include "os_type.h"
#include "osapi.h"
#include "driver/spi_interface.h"

#include "display.h"

// actual pixel dimensions of the LED array
#define SCREEN_HEIGHT 8
#define SCREEN_WIDTH 8

// Global variables are gross, but this one is necessary - stores the current state of the screen.
// Otherwise, the user would have to keep track of what's on the screen manually.
// It uses a bit of memory, but I think we can spare it for convenience.
// This could be just a uint64_t, but using the icon struct allows us to easily change the way icon
// storage is implemented in graphics.h if needed
icon_t cur_screen = {0, 8};

// serves as janky mutexes
// these are necessary - without checking if there's already a transition going, if a transition is
// called for while there's already one going, it will freeze. I don't know why yet, that's worth
// more investigation - it's quite possible it's avoidable without these.
struct mutex {
  volatile uint8_t screen        :1; // true when the screen is being actively written
  volatile uint8_t transition    :1; // true when there's a transition in progress
  volatile uint8_t queue         :1; // true when the queue is executing
  volatile uint8_t mutex3        :1; 
  volatile uint8_t mutex4        :1;
  volatile uint8_t mutex5        :1;
  volatile uint8_t mutex6        :1;
  volatile uint8_t mutex7        :1;
} mutex;

// the timer to drive a transition
static volatile os_timer_t trans_timer;

// current frame - this doesn't really need to be in the transition_data struct
uint8_t frame = 0;

// REMOVE ME - internal storage for a transition, for testing
transition_t internal_trans;



/* execute a single transition
 * trans: a struct defining the transition
 */
void update_screen( const icon_t image );
void ICACHE_FLASH_ATTR execute_transition() {
  icon_t test_icon = {0xFFFFFFFFFFFFFFFF, 8};
  update_screen(test_icon);
  update_screen(image_arr[SNOW]);
  return;
}



/* HERE BE DRAGONS
 * Functions below this line are low-level actual real-life (sorta) hardware functions - you
 * shouldn't need to touch them. These are the functions that actually shift the bits out to the
 * chip; there's nothing much going on down here logic-wise.
 */

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

// change the software-defined display brightness
void ICACHE_FLASH_ATTR display_brightness( uint8_t brightness ) {
  spi_transmit(0x0A, brightness);
  return;
}

// update the whole screen in one shot
// I'm not worrying about mutex locking...yet. I'll figure something out if it becomes a problem.
void ICACHE_FLASH_ATTR update_screen( const icon_t image ) {
  for( uint8_t i = 0x01; i <= 0x08; i++ ) {
    // this could probably be abstracted away a bit; for now, we know we're using uint64_t to
    // simulate an 8 member uint8_t array, so we'll keep this magic number for now...
    spi_transmit(i, (uint8_t)(image.icon >> ((i - 1) * 8)));
  }
  cur_screen = image;
  return;
}




// this is the actual transition "loop", which is really just a nonblocking timer function
void ICACHE_FLASH_ATTR transition_loop( void* tdata_raw ) {
  transition_t *data = (transition_t *)tdata_raw;

  // frame 0 is the current screen, 1 is the first transition frame, etc...
  frame++;

  // skip to the last frame if requested, artifically shifting cur_screen appropriately
  if( data->instant == true && frame <= data->space + data->icon.width ) {
    frame = data->space + data->icon.width;
    for( uint8_t i = 0; i < SCREEN_HEIGHT; i++ ) {
      ((uint8_t*)&cur_screen)[i] = ((uint8_t*)&cur_screen)[i] >> (data->space + data->icon.width - 1);
    }
  }

  // Generate the next frame then push it to the display in one shot.
  // This takes more time and memory, but it allows us to keep the cur_screen var updated.
  // Basically, I'm striving for screen state changes to be as atomic as possible, in that the state
  // is always known internally so the user doesn't have to worry about interrupting operations.
  icon_t next_frame;
  next_frame.width = SCREEN_WIDTH;
  for( uint8_t i = 0; i < SCREEN_HEIGHT; i++ ) {
    // treat the 64 bit numbers like an 8 member uint8_t array
    // shifts are 'backwards' because the MSB corresponds to the leftmost column and the LSB
    // corresponds to the rightmost column
    ((uint8_t*)&(next_frame.icon))[i] = 
      ((uint8_t*)&cur_screen)[i] >> 1 |
      ((uint8_t*)&data->icon)[i] << (SCREEN_WIDTH + data->space - frame);
  }
  update_screen(next_frame);

  if( frame == data->space + data->icon.width ) {
    // we've finished the transition!
    os_timer_disarm(&trans_timer);
    frame = 0;
  }
  return;
}
