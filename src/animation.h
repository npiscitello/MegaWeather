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

// persistent animation info - this needs to be a single global variable to allow the mutex to work
struct transition_data {
  uint64_t in_icon;         // new icon to be shown
  uint8_t frame;            // current frame of transition
  uint8_t space         :3; // cols of space between icons; for more than 7, use character[BLANK]
  uint8_t transitioning :1; // the screen is currently mid-transition; kind of a janky mutex
  uint8_t bool4         :1;
  uint8_t bool5         :1;
  uint8_t bool6         :1;
  uint8_t bool7         :1;
} tdata;

// the timer to drive a transition
static volatile os_timer_t trans_timer;



// transmit a register/data pair
#define spi_transmit(ADDR, DATA) spi_transaction(HSPI, 0, 0, 8, ADDR, 8, DATA, 0, 0);

// update the whole screen instantly
void update_screen( uint64_t image ) {
  for( uint8_t i = 0x01; i <= 0x08; i++ ) {
    spi_transmit(i, (uint8_t)(image >> ((i - 1) * 8)));
  }
  cur_screen = image;
}



// this is the actual animation "loop", which is really just a nonblocking timer function
// <TODO>: account for variable width icons (e.g. numbers)
void transition_loop( void* arg ) {
  (void)arg;
}



/* transition between 2 icons; 
 * icon: the icon to push to the screen
 * space: the number of blank columns between the current and new icons
 * delay: the frame period, in ms (this value determines how often the timer triggers)
 */
uint8_t transition( uint64_t icon, uint8_t space, uint16_t delay ) {
  if( !tdata.transitioning ) {
    tdata.in_icon = icon;
    tdata.space = space;

    tdata.frame = 0;

    os_timer_setfn(&transition_loop, (os_timer_func_t *)transition_loop, (void*)&tdata);
    os_timer_arm(&transition_loop, delay, 1);

    tdata.transitioning = true;

    return 0;
  } else {
    return 1;
  }
}

#endif
