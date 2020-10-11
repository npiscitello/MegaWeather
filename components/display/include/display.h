#ifndef DISPLAY_H
#define DISPLAY_H

// it hurts a little bit to have to #include so much in a header...
#include <stdint.h>
#include "graphics.h"

// defines a transition from one image to another
struct transition {
  icon_t icon;              // new icon to be shown
  uint8_t frame_delay;      // how long to wait between frames, in ms.
  uint8_t space         :3; // cols of space between icons; for more than 7, use character[BLANK]
  uint8_t instant       :1; // setting this to true will disable the slide animation
  uint8_t bool0         :1; 
  uint8_t bool1         :1;
  uint8_t bool2         :1;
  uint8_t bool3         :1;
};
typedef struct transition transition_t;



/* set up the display for initial first use; results in a completely blank display at half
 * brightness
 */
void display_init();

/* FOR TESTING ONLY */
void update_screen( const icon_t image );


/* set the display brightness
 * brightness: how intense the display is, 0x00 to 0x0F, higher is brighter
 */
void display_brightness(uint8_t brightness);



/* adds an item to the end of the transition queue. When flushed, this queue will be executed in
 * order with no time gap between entries.
 * item: the transition to execute, placed at the end of the queue
 * returns: true if the item was added successfully, false if not (the queue is probably full)
 */
uint8_t add_to_queue( transition_t* item );



/* flushes the queue, executing every transition with no time delay and clearing the queue. If this
 * is called while there's already a queue executing, the previous run will be cancelled and the new
 * one will immediately commence. Note, however, that the queue will contain the same transitions
 * unless it is manually cleared (which has the side effect of cancelling a currently running queue)
 * returns: true if the queue was flushed, false if the transitions were unable to execute. If this
 *    function returns false, the queue remains intact (i.e, it has not been cleared) and calling
 *    this function again should produce the expected transition, provided the previous transition
 *    has ended.
 */
void execute_queue();



/* clears the queue without executing any transitions. This can also be used to cancel a queue
 * that's already running.
 */
void clear_queue();



/* checks if a queue is being executed
 * returns: true if the queue is being executed, false if not. Note, this doesn't guarantee the
 * screen isn't changing, just that the queue is not being executed.
 */
uint8_t queue_executing();

#endif
