#ifndef ANIMATION_H
#define ANIMATION_H

#include <avr/io.h>
#include <stdint.h>

// data for slide transition
struct transition_data {
  uint64_t  out_icon;       // icon to be replaced
  uint64_t  in_icon[3];     // new icons to be shown (in_icon[0] used if icon_slide == true)
  uint8_t   number;         // number reflected in in_icon
  uint8_t   frame;          // current frame of the transition
  uint8_t   space       :3; // space between icons; for more than 7 columns, use character[BLANK]
  uint8_t   icon        :1; // true if transitioning between 2 icons, false if displaying a number
  uint8_t   trans_done  :1; // slide finished flag
  uint8_t   frame_ready :1; // true if the inter-frame time has elapsed
  uint8_t   bool6       :1;
  uint8_t   bool7       :1;
};
typedef struct transition_data transition_data;

// these aren't defined
#define false 0
#define true  1

// how long to delay between frames in a slide transition, in ms (0-255)
#define SLIDE_DELAY 100

// these make more sense in the graphics header but they're only used by animation functions and I
// don't want to make this file dependent on the graphics header
#define ICON_W  8
#define NUM_W   6



// update a register on the MAX72XX
void transmit(const uint8_t reg, const uint8_t val) {
  // set CS low
  PORTB &= 0xFF & !_BV(PORTB2);
  // what register to write on the MAX7221
  SPDR = reg;
  // wait for the transmission to finish (poll the finish flag)
  // this could be interrupt based but that would be needlessly
  // complicated and actually slower!
  while( !(SPSR & _BV(SPIF)) ) {}
  // what to write into the register on the MAX7221
  SPDR = val;
  // wait for the transmission to finish (poll the finish flag)
  while( !(SPSR & _BV(SPIF)) ) {}
  // bring CS high
  PORTB |= 0x00 | _BV(PORTB2);
  return;
}



// this does all the work - it is to be called in a loop to generate the transition
void transition_loop(transition_data* tdata) {
  TIFR0 |= _BV(OCF0A);
  tdata->frame++;
  
  /* we need three if statements because, with just one, the arguments to the left shifts would become
   * negative. This could probably be mitigated with some macro magic, but I am no magician.
   * Besides, I find this way clearer. */

  // different loops depending on if we're transitioning between icons or a set of numbers.
  if( tdata->icon ) {
    if( tdata->frame <= tdata->space + ICON_W ) {
      // treat the icon like an 8-member uint8_t array
      // the shifts are 'backwards'; this is due to the writes being from right to left
      for( uint8_t j = 0x01; j < 0x09; j++ ) {
        transmit(j, ((((uint8_t*)&tdata->out_icon)[j - 1]) >> tdata->frame) | 
            ((((uint8_t*)&tdata->in_icon)[j - 1]) << (tdata->space + ICON_W - tdata->frame)));
      }

    } else {
      // this means we are at our last frame
      TCCR0B &= (0xFF & !_BV(CS02) & !_BV(CS00));
      tdata->trans_done = true;
    }

  } else {
    // wipe across the first space + ICON_W columns (clear icon)
    if( tdata->frame <= tdata->space + ICON_W ) {
      for( uint8_t j = 0x01; j < 0x09; j++ ) {
        transmit(j, ((((uint8_t*)&tdata->out_icon)[j - 1]) >> tdata->frame) | 
            ((((uint8_t*)&tdata->in_icon[0])[j - 1]) << (tdata->space + ICON_W - tdata->frame)) |
            ((((uint8_t*)&tdata->in_icon[1])[j - 1]) << (tdata->space + ICON_W + NUM_W - tdata->frame)));
      }

    } else if( tdata->frame <= tdata->space + ICON_W + NUM_W
        && tdata->frame > tdata->space + ICON_W
        && tdata->number > 9) {
      // wipe across the next NUM_W columns (clear first digit)
      for( uint8_t j = 0x01; j < 0x09; j++ ) {
        transmit(j, ((((uint8_t*)&tdata->in_icon[0])[j - 1]) >> (tdata->frame - (tdata->space + ICON_W))) | 
            ((((uint8_t*)&tdata->in_icon[1])[j - 1]) << (tdata->space + ICON_W + NUM_W - tdata->frame)) |
            ((((uint8_t*)&tdata->in_icon[2])[j - 1]) << (tdata->space + ICON_W + NUM_W + NUM_W - tdata->frame)));
      }

    } else if( tdata->frame <= tdata->space + ICON_W + NUM_W + NUM_W
        && tdata->frame > tdata->space + ICON_W + NUM_W
        && tdata->number > 99 ) {
      // wipe across the final NUM_W columns (clear second digit)
      for( uint8_t j = 0x01; j < 0x09; j++ ) {
        transmit(j, ((((uint8_t*)&tdata->in_icon[1])[j - 1]) >> (tdata->frame - (tdata->space + ICON_W + NUM_W))) | 
            ((((uint8_t*)&tdata->in_icon[2])[j - 1]) << (tdata->space + ICON_W + NUM_W + NUM_W - tdata->frame)));
      }

    } else {
      // this means that all of the digits have scrolled. Disable the timer and clean up
      TCCR0B &= (0xFF & !_BV(CS02) & !_BV(CS00));
      tdata->trans_done = true;
    }
  }
  return;
}



/* slide transition between icons
 *  tdata:    pointer to the data struct to store the transition info in
 *  out_icon: old icon to be replaced
 *  in_icon:  new icon to be displayed
 *  space:    number of blank columns between icons */
void icon_transition(transition_data* tdata,
    const uint64_t out_icon,
    const uint64_t in_icon,
    const uint8_t space) {
  tdata->icon = true;
  tdata->out_icon = out_icon;
  tdata->in_icon[0] = in_icon;
  tdata->space = space;

  // kick off the first frame
  tdata->frame = 0;
  transition_loop(tdata);

  /* set the timer value to zero before starting the clock to ensure we wait the full period
   * before triggering the second frame. /1024 prescaler on a 1MHz clock give a tick of ~1ms.
   * Poll the timer overflow flag in TIFR0 to determine when the timer has expired. */
  TCNT0 = 0x00;
  TCCR0B |= (_BV(CS02) | _BV(CS00));
  return;
}



/* slide transition to show a number. It will stop on the last number and return the 
 * index of that digit; this can be used to transition from that digit back to an icon.
 *  tdata:    pointer to the data struct to store the transition info in
 *  out_icon: icon to be replaced
 *  number:   number to be displayed. This is passed as an integer that will be parsed.
 *  space:    number of blank columns between icon and number (0-7)
 * 
 * this is the nonblocking version of the above function */
// <TODO> degree sign? Is this necessary?
uint8_t number_transition(transition_data* tdata,
    const uint64_t out_icon,
    const uint8_t space) {
  tdata->icon = false;
  tdata->out_icon = out_icon;
  tdata->space = space;
  
  // kick off the first frame
  tdata->frame = 0;
  transition_loop(tdata);

  /* set the timer value to zero before starting the clock to ensure we wait the full period
   * before triggering the second frame. /1024 prescaler on a 1MHz clock give a tick of ~1ms.
   * Poll the timer overflow flag in TIFR0 to determine when the timer has expired. */
  TCNT0 = 0x00;
  TCCR0B |= (_BV(CS02) | _BV(CS00));
  return tdata->number % 10;
}



// blocks until any currently running transitions are done
void wait_for_transition(transition_data* tdata) {
  while( !tdata->trans_done ) {
    if( TIFR0 & _BV(OCF0A) ) {
      transition_loop(tdata);
    }
  }
  tdata->trans_done = false;
}

#endif
