#ifndef DISPLAY_H
#define DISPLAY_H

// it hurts a little bit to have to #include so much in a header...
#include <stdint.h>
#include "graphics.h"

// How important it is to write to the screen, compared to other tasks. Values
// can be 1-11, higher is more important.
#define DISPLAY_PRIORITY 10

// Defines a transition from one image to another.
typedef struct {
  icon_t icon;              // new icon to be shown
  uint8_t frame_delay;      // how long to wait between animation frames, in ms
  uint8_t icon_delay;       // how long to wait until starting the next queued
                            // transition, in 10s of ms (15 = 150ms delay)
  uint8_t space         :3; // cols of space between icons; for more than 7,
                            // use character[BLANK]
  uint8_t instant       :1; // setting this to true will disable the slide
                            // animation
  uint8_t bool3         :1;
  uint8_t bool2         :1;
  uint8_t bool1         :1;
  uint8_t bool0         :1; 
} transition_t;

// Defines an executable collection of transitions. Members marked (internal)
// are manipulated by the driver internally and can be ignored.
// If you're just using the queue_append_single and queue_clear_single 
// functions, you don't have to worry about this struct!
typedef struct {
  transition_t* ptr;  // the malloc-ed storage for the queue
  uint8_t length;     // number of items currently in the queue
  uint8_t max_length; // number of items allowed to be in the queue (internal)
  uint8_t index;      // current location in the queue (internal)
} queue_t;

// return codes
typedef enum {
  RET_NO_ERR,           // function completed successfully
  RET_UNKNOWN_ERR,      // an unhandled error occurred
  RET_BAD_MALLOC,       // memory allocation failed
  RET_QUEUE_FULL,       // the queue is full
  RET_QUEUE_TRUNC,      // the operation was successful but the queue was truncated
  RET_QUEUE_LOCKED      // the queue cannot be accessed
} ret_code_t;



/* Sets up the display for use; results in a completely blank display at half
 * brightness. Also sets up memory for the transition queue.
 * queue_size: how many transitions the internal driver queue can store. This
 *    is limited by available memory. (NOTE: this value only affects the
 *    driver's interal queue. Custom queues created in user code can be longer)
 */
ret_code_t disp_driver_init(const uint8_t queue_size);



/* Sets the display brightness.
 * brightness: how intense the display is, 0x00 to 0x0F, higher is brighter
 */
ret_code_t disp_set_bright(const uint8_t brightness);



/* Adds a single transition to the end of the queue. If the queue cannot be
 * accessed (being written to or being shifted out to the screen), returns
 * RET_QUEUE_LOCKED.
 * item: the transition to execute, placed at the end of the queue
 */
ret_code_t disp_queue_append_single(transition_t* item);



/* Remove a single transition from the end of the queue. If called while the
 * queue is empty, this function will have no effect. If the queue cannot be
 * accessed (being written to or being shifted out to the screen), returns
 * RET_QUEUE_LOCKED.
 */
ret_code_t disp_queue_clear_single();



/* Copies a queue into the driver, appending to any existing contents. If there
 * are more transitions than the driver has been configured for, the queue will
 * be truncated. The provided queue is safe to be overwritten or freed after
 * this function is called. If you want to fill the queue from scratch, call
 * queue_clear() first. If the queue cannot be accessed (being written to or
 * being shifted out to the screen), returns RET_QUEUE_LOCKED.
 * queue: the user-constructed queue to be copied into the driver
 */
ret_code_t disp_queue_append(queue_t* queue);



/* Removes all transitions from the queue. If the queue cannot be accessed
 * (being written to or being shifted out to the screen), returns
 * RET_QUEUE_LOCKED.
 */
ret_code_t disp_queue_clear();



/* Resets the queue index to the beginning of the queue (useful if the queue was
 * stopped before completion but you want to start a new queue from the
 * beginning)
 */
ret_code_t disp_queue_reset();



/* Starts the queue execution. If the queue is already started, this has no
 * effect. Queue automatically stops and resets when it reaches the end. If
 * the queue is started while the current index is beyond the end of the queue,
 * the queue will start from the beginning.
 */
ret_code_t disp_queue_start();



/* Stops the queue execution. The queue may be started again from where it left
 * off with queue_start. If the queue is already stopped, this has no effect.
 */
ret_code_t disp_queue_stop();

#endif
