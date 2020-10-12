#include <stdio.h>

#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_attr.h"
#include "driver/gpio.h"
#include "driver/spi.h"
#include "driver/hw_timer.h"


#include "display.h"
#include "graphics.h"

// actual pixel dimensions of the LED array
#define SCREEN_HEIGHT 8
#define SCREEN_WIDTH 8

// how many transitions can be queued at once
#define MAX_QUEUE_LENGTH 5

// global variables are gross, but this one is necessary - stores the current state of the screen
// otherwise, the user would have to keep track of what's on the screen manually
// uses a bit of memory, but I think we can spare it for convenience
// this could be just a uint64_t, but using the icon struct allows us to easily change the way icon
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

// Does this need to be in a struct? Probably not, but it feels cleaner - I hate standalone globals.
// I mean, I hate globals in general, so...
struct transition_queue {
  transition_t transitions[MAX_QUEUE_LENGTH];  // the actual queue storage
  uint8_t length;                                   // how many items are currently in the queue
  uint8_t current_index;                            // which item we're executing
} queue;

/*
// the timer to drive a transition
static volatile os_timer_t trans_timer;

// current frame - this doesn't really need to be in the transition_data struct
uint8_t frame = 0;



// add a transition to the queue if there's space
uint8_t ICACHE_FLASH_ATTR add_to_queue( transition_t* item ) {
  if( queue.length < MAX_QUEUE_LENGTH ) {
    os_memcpy(&(queue.transitions[queue.length]), item, sizeof(transition_t));
    queue.length++;
    return true;
  } else {
    return false;
  }
}



// reset the queue without executing any queued transitions
void ICACHE_FLASH_ATTR clear_queue() {
  // we don't really need to worry about actually clearing the memory - it will get overwritten if
  // it needs to be
  queue.current_index = 0;
  queue.length = 0;
  return;
}



// I need to declare these functions to be able to use it
void transition_loop( void* tdata_raw );
void update_screen( const icon_t image );
// execute queued transitions and clear the queue
void ICACHE_FLASH_ATTR queue_helper() {

  // clean up after the previous run; having these here instead of in transition_loop allows us to
  // interrupt queue executions with other queue executions
  os_timer_disarm( &trans_timer );
  frame = 0;

  if( queue.current_index < queue.length ) {
    os_timer_setfn(&trans_timer, (os_timer_func_t *)transition_loop, 
        (void*)&(queue.transitions[queue.current_index]));
    os_timer_arm(&trans_timer, queue.transitions[queue.current_index].frame_delay, 1);

    // kick off the first frame manually, fixes the timing glitch
    transition_loop( (void*)&(queue.transitions[queue.current_index]) );

    queue.current_index++;

  } else {
    // we're done with the queue
    clear_queue();
    mutex.queue = false;
  }

  return;
}



void ICACHE_FLASH_ATTR execute_queue() {
  queue.current_index = 0;
  mutex.queue = true;
  queue_helper();
}



uint8_t ICACHE_FLASH_ATTR queue_executing() {
  return mutex.queue;
}
*/



/* HERE BE DRAGONS
 * Functions below this line are low-level actual real-life (sorta) hardware functions - you
 * shouldn't need to touch them. These are the functions that actually shift the bits out to the
 * chip; there's nothing much going on down here logic-wise.
 */

/* SPI RTOS Scheduling Theory
 * <TODO> right now, SPI is blocking. Use the below scheduling details to
 * implement non-blocking SPI!
 *
 * schedule SPI task as a fairly high priority (priorities are 1-11, lower
 * numbers being less important) - shifting to the display is the most important
 * thing we do, this being a smart display and all. We use the semaphore to tell
 * the OS that there's SPI data ready to write out and it should give SPI some
 * processor time.
 *
 * SPI will also supposedly spit out "events" (ISRs) when it starts sending and
 * finishes sending data (according to itr_enable). Maybe these are useful?
 */

// semaphore for locking SPI while a transmit is happening
//SemaphoreHandle_t sem_spi_transmit = NULL;

// catch SPI events
void spi_event_callback(int event, void *arg) {
  // do stuff? Event macros are SPI_X_EVENT, where X is:
  // INIT, TRANS_START, TRANS_DONE, DEINIT
  switch( event ) {
    case SPI_INIT_EVENT:
      printf("[disp] SPI initialized\n");
      //xSemaphoreGiveFromISR(sem_spi_transmit, NULL);
      break;
    case SPI_TRANS_START_EVENT:
      printf("[disp] SPI transmission started\n");
      break;
    case SPI_TRANS_DONE_EVENT:
      printf("[disp] SPI transmission ended\n");
      //xSemaphoreGiveFromISR(sem_spi_transmit, NULL);
      break;
    case SPI_DEINIT_EVENT:
      printf("[disp] SPI deinitialized\n");
      break;
  }
}

// transmit a register/data pair
//#define spi_transmit(ADDR, DATA) spi_transaction(HSPI, 0, 0, 8, ADDR, 8, DATA, 0, 0);
void spi_transmit(uint8_t addr, uint8_t data) {
  spi_trans_t spi_packet;

  uint32_t addr_cpy = addr;
  uint32_t data_cpy = data;

  //spi_packet.bits.addr = sizeof(addr) * 8;
  spi_packet.bits.addr = 8;
  spi_packet.addr = &addr_cpy;
  //spi_packet.bits.mosi = sizeof(data) * 8;
  spi_packet.bits.mosi = 8;
  spi_packet.mosi = &data_cpy;

  spi_packet.bits.cmd = 0;
  spi_packet.bits.miso = 0;

  //xSemaphoreTake(sem_spi_transmit, 1 * portTICK_PERIOD_MS);
  spi_trans(HSPI_HOST, &spi_packet);
}

// set up the screen and other variables for first use
void ICACHE_FLASH_ATTR display_init() {
  // init the queue
  queue.length = 0;
  queue.current_index = 0;

  // set up the SPI transmit semaphore
  //sem_spi_transmit = xSemaphoreCreateBinary();

  // configure SPI
  spi_config_t spi_config_data;
  // CS_EN:1, MISO_EN:1, MOSI_EN:1, BYTE_TX_ORDER:1, BYTE_TX_ORDER:1,
  // BIT_RX_ORDER:0, BIT_TX_ORDER:0, CPHA:0, CPOL:0
  //spi_config_data.interface.val = SPI_DEFAULT_INTERFACE;
  spi_config_data.interface.cs_en = 1;
  spi_config_data.interface.mosi_en = 1;
  spi_config_data.interface.miso_en = 0;
  spi_config_data.interface.byte_tx_order = 1;
  spi_config_data.interface.bit_tx_order = 0;
  spi_config_data.interface.cpha = 0;
  spi_config_data.interface.cpol = 0;
  // TRANS_DONE: true, WRITE_STATUS: false, READ_STATUS: false,
  // WRITE_BUFFER: false, READ_BUFFER: false
  //spi_config_data.intr_enable.val = SPI_MASTER_DEFAULT_INTR_ENABLE;
  spi_config_data.intr_enable.val = 0;
  spi_config_data.mode = SPI_MASTER_MODE;
  spi_config_data.clk_div = SPI_2MHz_DIV;
  spi_config_data.event_cb = spi_event_callback;

  // use the external (not-flash) pins
  spi_init(HSPI_HOST, &spi_config_data);

  // setup for the MAX7221 chip (through a TXB0104 level shifter)
  // <TODO> DEBUG ONLY - display test mode
  spi_transmit(0x0F, 0x01);
  // don't use the decode table
  //spi_transmit(0x09, 0x00);
  // set intensity to middle ground
  //spi_transmit(0x0A, 0x08);
  // scan across all digits
  //spi_transmit(0x0b, 0x07);
  // turn off all pixels - I could use update_screen for this, but I don't need the fancy shifting
  // I'll probably start using it if I need to worry about mutexes
  for( uint8_t i = 0x01; i <= 0x08; i++ ) {
    //spi_transmit(i, 0x00);
  }
  // take the chip out of shutdown
  //spi_transmit(0x0C, 0x01);
  // <TODO> DEBUG make something show up on the LEDs
  for( uint8_t i = 0; i < 100; i++ ) {
    spi_transmit(0xFF, 0xFF);
  }
}

// change the software-defined display brightness
void ICACHE_FLASH_ATTR display_brightness( uint8_t brightness ) {
  spi_transmit(0x0A, brightness);
}

// update the whole screen in one shot
// I'm not worrying about mutex locking...yet. I'll figure something out if it becomes a problem.
void ICACHE_FLASH_ATTR update_screen( const icon_t image ) {
  for( uint8_t i = 0x01; i <= 0x08; i++ ) {
    // this could probably be abstracted away a bit; for now, we know we're using uint64_t to
    // simulate an 8 member uint8_t array, so we'll keep this magic number for now...
    //spi_transmit(i, (uint8_t)(image.icon >> ((i - 1) * 8)));
  }
  cur_screen = image;
}



/*
// this is the actual transition "loop", which is really just a nonblocking timer function
void ICACHE_FLASH_ATTR transition_loop( void* tdata_raw ) {
  transition_t *data = (transition_t *)tdata_raw;
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
  if( frame <= data->space + data->icon.width ) {

    icon_t next_frame;
    next_frame.width = 8;
    for( uint8_t i = 0; i < SCREEN_HEIGHT; i++ ) {
      // treat the 64 bit numbers like an 8 member uint8_t array
      // shifts are 'backwards' because the MSB corresponds to the leftmost column and the LSB
      // corresponds to the rightmost column
      ((uint8_t*)&next_frame)[i] = 
        ((uint8_t*)&cur_screen)[i] >> 1 |
        ((uint8_t*)&data->icon)[i] << (SCREEN_WIDTH + data->space - frame);
    }
    update_screen(next_frame);

  } else {
    // we've finished the transition! Let the queue know we're done
    queue_helper();
  }
}
*/
