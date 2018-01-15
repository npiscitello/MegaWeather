#include "osapi.h"
#include "driver/spi.h"

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
/*
struct mutex {
  volatile uint8_t screen        :1; // true when the screen is being actively written
  volatile uint8_t transition    :1; // true when there's a transition in progress
  volatile uint8_t mutex2        :1; 
  volatile uint8_t mutex3        :1; 
  volatile uint8_t mutex4        :1;
  volatile uint8_t mutex5        :1;
  volatile uint8_t mutex6        :1;
  volatile uint8_t mutex7        :1;
} mutex;
*/

// Does this need to be in a struct? Probably not, but it feels cleaner - I hate standalone globals.
// I mean, I hate globals in general, so...
struct transition_queue {
  transition_data_t transitions[MAX_QUEUE_LENGTH];  // the actual queue storage
  uint8_t length;                                   // how many items are currently in the queue
  uint8_t current_index;                            // which item we're executing
} queue;

// the timer to drive a transition
static volatile os_timer_t trans_timer;

// current frame - this doesn't really need to be in the transition_data struct
uint8_t frame = 0;



// add a transition to the queue if there's space
uint8_t ICACHE_FLASH_ATTR add_to_queue( transition_data_t* item ) {
  if( queue.length < MAX_QUEUE_LENGTH ) {
    os_memcpy(&queue.transitions + (queue.length), item, sizeof(transition_data_t));
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



// I need to declare this function to be able to use it
void transition_loop( void* tdata_raw );
// execute queued transitions and clear the queue
void ICACHE_FLASH_ATTR execute_queue() {

  if( queue.current_index < queue.length ) {
    os_timer_setfn(&trans_timer, (os_timer_func_t *)transition_loop, 
        (void*)&(queue.transitions[queue.current_index]));
    os_timer_arm(&trans_timer, queue.transitions[queue.current_index].frame_delay, 1);
    queue.current_index++;

  } else {
    clear_queue();
  }

  return;
}



/* HERE BE DRAGONS
 * Functions below this line are low-level actual real-life (sorta) hardware functions - you
 * shouldn't need to touch them. These are the functions that actually shift the bits out to the
 * chip; there's nothing much going on down here logic-wise.
 */

// transmit a register/data pair
#define spi_transmit(ADDR, DATA) spi_transaction(HSPI, 0, 0, 8, ADDR, 8, DATA, 0, 0);

// set up the screen and other variables for first use
void ICACHE_FLASH_ATTR display_init() {

  // init the queue
  queue.length = 0;
  queue.current_index = 0;

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
  // turn off all pixels - I could use update_screen for this, but I don't need the fancy shifting
  // I'll probably start using it if I need to worry about mutexes
  for( uint8_t i = 0x01; i <= 0x08; i++ ) {
    spi_transmit(i, 0x00);
  }
  // take the chip out of shutdown
  spi_transmit(0x0C, 0x01);
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
    spi_transmit(i, (uint8_t)(image.icon >> ((i - 1) * 8)));
  }
  cur_screen = image;
}



// this is the actual transition "loop", which is really just a nonblocking timer function
void ICACHE_FLASH_ATTR transition_loop( void* tdata_raw ) {
  transition_data_t *data = (transition_data_t *)tdata_raw;
  frame++;

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
    // we've finished the transition! Clean up and let the queue know we're done
    os_timer_disarm(&trans_timer);
    frame = 0;
    execute_queue();
  }
}
