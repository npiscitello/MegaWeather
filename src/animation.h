#ifndef ANIMATION_H
#define ANIMATION_H

#include "osapi.h"
#include "driver/spi.h"

// these make more sense in the graphics header but they're only used by animation functions and I
// don't want to make this file dependent on the graphics header
#define ICON_W  8
#define NUM_W   6

// how long to delay between frames in a slide transition, in ms (0-255)
#define DEFAULT_DELAY 100

// global variables are gross, but this one is necessary - stores the current state of the screen
// otherwise, the user would have to keep track of what's on the screen manually
// uses a bit of memory, but I think we can spare it for convenience
// in main.c, we start up to a blank screen
uint64_t cur_screen = 0;

// serves as kind of a janky mutex to lock screen writing
uint8_t screen_lock = 0;

struct transition_data {
  uint64_t in_icon;         // new icon to be shown
  uint8_t frame_no;         // current frame of transition
  uint8_t space         :3; // cols of space between icons; for more than 7, use character[BLANK]
  uint8_t bool3         :1; 
  uint8_t bool4         :1;
  uint8_t bool5         :1;
  uint8_t bool6         :1;
  uint8_t bool7         :1;
};
typedef struct transition_data transition_data;

// the timer to drive a transition
static volatile os_timer_t trans_timer;



// transmit a register/data pair
#define spi_transmit(ADDR, DATA) spi_transaction(HSPI, 0, 0, 8, ADDR, 8, DATA, 0, 0);

// update the whole screen in one shot
void ICACHE_FLASH_ATTR update_screen( uint64_t image ) {
  for( uint8_t i = 0x01; i <= 0x08; i++ ) {
    spi_transmit(i, (uint8_t)(image >> ((i - 1) * 8)));
  }
  cur_screen = image;
}



// this is the actual animation "loop", which is really just a nonblocking timer function
// <TODO>: account for variable width icons (e.g. numbers)
void ICACHE_FLASH_ATTR transition_loop( void* tdata_raw ) {
  transition_data *tdata = (transition_data *)tdata_raw;
  (tdata->frame_no)++;
  // Generate the next frame then push it to the display in one shot.
  // This takes more time and memory, but it allows us to keep the cur_screen var updated.
  // That will allow us, in the future, to cancel animations, interrupt with new animations, and
  // other cool stuff like that.
  // Basically, I'm striving for screen state changes to be as atomic as possible, in that the state
  // is always known internally so the user doesn't have to worry about interrupting operations.
  if( tdata->frame_no <= tdata->space + ICON_W ) {
    uint64_t next_frame;
    for( uint8_t i = 0; i < 8; i++ ) {
      // treat the 64 bit numbers like an 8 member uint8_t array
      ((uint8_t*)next_frame)[i] = 
        ((uint8_t*)cur_screen)[i] << tdata->frame_no |
        ((uint8_t*)tdata->in_icon)[i] >> (ICON_W + tdata->space - tdata->frame_no);
    }
    update_screen(next_frame);
  } else {
    // we're done! disable the timer and free the memory we used for the tdata struct
    os_timer_disarm(&trans_timer);
    free(tdata);
  }
}



/* transition between 2 icons; 
 * icon: the icon to push to the screen
 * space: the number of blank columns between the current and new icons
 * delay: the frame period, in ms (this value determines how often the timer triggers)
 */
uint8_t ICACHE_FLASH_ATTR transition( uint64_t icon, uint8_t space, uint16_t delay ) {
  if( !screen_lock ) {
    // make sure to free this memory before you start another transition!
    // there's probably a better way to do this, but I don't want to use a global var because I
    // don't want the struct to be able to be changed mid-transition
    // I forgot - no malloc without an OS!
    transition_data *tdata = malloc(sizeof(transition_data));
    tdata->in_icon = icon;
    tdata->space = space;

    tdata->frame_no = 0;

    os_timer_setfn(&transition_loop, (os_timer_func_t *)transition_loop, (void*)tdata);
    os_timer_arm(&transition_loop, delay, 1);

    screen_lock = true;

    return 0;
  } else {
    return 1;
  }
}

#endif
