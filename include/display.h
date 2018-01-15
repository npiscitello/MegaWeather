#ifndef DISPLAY_H
#define DISPLAY_H

// it hurts a little bit to have to #include so much in a header...
#include "c_types.h"
#include "graphics.h"

// defines a transition from one image to another
struct transition_data {
  icon_t icon;              // new icon to be shown
  //uint8_t frame_no;         // current frame of transition
  uint8_t frame_delay;      // how long to wait between frames, in ms
  uint8_t space         :3; // cols of space between icons; for more than 7, use character[BLANK]
  uint8_t bool0         :1; 
  uint8_t bool1         :1;
  uint8_t bool2         :1;
  uint8_t bool3         :1;
  uint8_t bool4         :1;
};
typedef struct transition_data transition_data_t;



/* set up the display for initial first use; results in a completely blank display at half
 * brightness
 */
void display_init();


/* set the display brightness
 * brightness: how intense the display is, 0x00 to 0x0F, higher is brighter
 */
void display_brightness(uint8_t brightness);



/* adds an item to the end of the transition queue. When flushed, this queue will be executed in
 * order with no time gap between entries.
 * item: the transition to execute, placed at the end of the queue
 * returns: true if the item was added successfully, false if not (the queue is probably full)
 */
uint8_t add_to_queue( transition_data_t* item );



/* flushes the queue, executing every transition with no time delay and clearing the queue
 * returns: true if the queue was flushed, false if the transitions were unable to execute. If this
 *    function returns false, the queue remains intact (i.e, it has not been cleared) and calling
 *    this function again should produce the expected transition, provided the previous transition
 *    has ended.
 */
uint8_t flush_queue();



/* clears the queue without executing any transitions
 */
void clear_queue();

#endif
