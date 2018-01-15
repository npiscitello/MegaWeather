#ifndef ANIMATION_H
#define ANIMATION_H

#include "osapi.h"
#include "driver/spi.h"
#include "mem.h"
#include "graphics.h"

#define SCREEN_HEIGHT 8
#define SCREEN_WIDTH 8

// how long to delay between frames in a slide transition, in ms (0-255)
#define DEFAULT_DELAY 100

// global variables are gross, but this one is necessary - stores the current state of the screen
// otherwise, the user would have to keep track of what's on the screen manually
// uses a bit of memory, but I think we can spare it for convenience
// in main.c, we start up to a blank screen
// this could be just a uint64_t, but using the icon struct allows us to easily change the way icon
// storage is implemented in graphics.h if needed
icon_t cur_screen = {0, 8};

// serves as janky mutexes
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

// I'd rather malloc and free space for this, but the nonos API malloc acts strangely...
struct transition_data {
  icon_t icon;              // new icon to be shown
  uint8_t frame_no;         // current frame of transition
  uint8_t space         :3; // cols of space between icons; for more than 7, use character[BLANK]
  uint8_t bool0         :1; 
  uint8_t bool1         :1;
  uint8_t bool2         :1;
  uint8_t bool3         :1;
  uint8_t bool4         :1;
} tdata;
typedef struct transition_data transition_data;

// the timer to drive a transition
static volatile os_timer_t trans_timer;



// transmit a register/data pair
#define spi_transmit(ADDR, DATA) spi_transaction(HSPI, 0, 0, 8, ADDR, 8, DATA, 0, 0);

// update the whole screen in one shot
void ICACHE_FLASH_ATTR update_screen( const icon_t image ) {
  mutex.screen = true;
  for( uint8_t i = 0x01; i <= 0x08; i++ ) {
    // this could probably be abstracted away a bit; for now, we know we're using uint64_t to
    // simulate an 8 member uint8_t array, so we'll keep this magic number for now...
    spi_transmit(i, (uint8_t)(image.icon >> ((i - 1) * 8)));
  }
  cur_screen = image;
  mutex.screen = false;
}



// this is the actual animation "loop", which is really just a nonblocking timer function
// <TODO>: account for variable width icons (e.g. numbers)
void ICACHE_FLASH_ATTR transition_loop( void* tdata_raw ) {
  transition_data *data = (transition_data *)tdata_raw;
  (data->frame_no)++;

  // Generate the next frame then push it to the display in one shot.
  // This takes more time and memory, but it allows us to keep the cur_screen var updated.
  // That will allow us, in the future, to cancel animations, interrupt with new animations, and
  // other cool stuff like that.
  // Basically, I'm striving for screen state changes to be as atomic as possible, in that the state
  // is always known internally so the user doesn't have to worry about interrupting operations.
  if( data->frame_no <= data->space + data->icon.width ) {

    icon_t next_frame;
    next_frame.width = 8;
    for( uint8_t i = 0; i < SCREEN_HEIGHT; i++ ) {
      // treat the 64 bit numbers like an 8 member uint8_t array
      // shifts are 'backwards' because the MSB corresponds to the leftmost column and the LSB
      // corresponds to the rightmost column
      ((uint8_t*)&next_frame)[i] = 
        ((uint8_t*)&cur_screen)[i] >> 1 |
        ((uint8_t*)&data->icon)[i] << (SCREEN_WIDTH + data->space - data->frame_no);
    }
    update_screen(next_frame);

  } else {
    os_timer_disarm(&trans_timer);
    mutex.transition = false;
  }
}



/* slide transition between 2 icons; 
 * icon: the icon to push to the screen
 * space: the number of blank columns between the current and new icons
 * delay: the frame period, in ms (this value determines how often the frame timer triggers)
 *
 * returns: true if the transition was started, false if not
 */
uint8_t ICACHE_FLASH_ATTR transition( 
    icon_t icon, 
    uint8_t space, 
    uint16_t delay ) {
  if( !mutex.transition ) {
    tdata.icon = icon;
    tdata.space = space;

    tdata.frame_no = 0;

    os_timer_setfn(&trans_timer, (os_timer_func_t *)transition_loop, (void*)&tdata);
    os_timer_arm(&trans_timer, delay, 1);

    mutex.transition = true;

    return true;
  } else {
    return false;
  }
}



// <TODO> create an API call to fluidly display an arbitrary length string of characters
/* apply a nonstop slide for an arbitrary number of icons of arbitrary widths, like a ticker
 * icon_arr: array of icon structs in the order they will be pushed to the screen
 * num_icons: the number of items in icon_arr
 * space: the space to put in between each icon
 * delay: the frame period, in ms (this value determines how often the frame timer triggers)
 *
 * returns: true if the transition was started, false if not
 */
uint8_t ICACHE_FLASH_ATTR transition_multiple(
    uint64_t* icon_arr,
    uint8_t num_icons,
    uint8_t space,
    uint16_t delay ) {
  return false;
}



/* check to see if there's a transition happening
 *
 * returns: true if there's a transition running, false if not
 */
uint8_t ICACHE_FLASH_ATTR transition_running() {
  return mutex.transition;
}

#endif
