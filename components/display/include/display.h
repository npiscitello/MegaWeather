#ifndef DISPLAY_H
#define DISPLAY_H

// it hurts a little bit to have to #include so much in a header...
#include <stdint.h>
#include "graphics.h"

// Defines a transition from one image to another.
typedef struct {
  icon_t icon;              // new icon to be shown
  uint8_t frame_delay;      // how long to wait between frames, in ms.
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
// are manipulated by the driver and can be ignored.
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
  RET_QUEUE_EXECUTING,  // the queue is executing
  RET_QUEUE_STOPPED     // the queue is not executing
} ret_code_t;



/* Sets up the display for use; results in a completely blank display at half
 * brightness. Also sets up memory for the transition queue.
 * queue_size: how many transitions the queue can store. This is limited by
 *    available memory (and keep in mind - the queue is duplicated internally
 *    when executed)
 */
ret_code_t driver_init(const uint8_t queue_size);



/* Sets the display brightness.
 * brightness: how intense the display is, 0x00 to 0x0F, higher is brighter
 */
ret_code_t display_set_bright(const uint8_t brightness);



/* Adds an item to the end of the transition queue. Safe to call while a
 * previously defined queue is executing.
 * item: the transition to execute, placed at the end of the queue
 */
ret_code_t queue_add(transition_t* item);



/* Remove a transition from the end of the transition queue. Safe to call while
 * a previously defined queue is executing.
 */
ret_code_t queue_remove();



/* Copies a queue into the driver, appending to any existing contents. If there
 * are more transitions than the driver has been configured for, the queue will
 * be truncated. Safe to call while a previously defined queue is executing.
 * The provided queue is safe to be overwritten or freed after this function
 * is called.
 * queue: the user-constructed queue to be copied into the driver
 */
ret_code_t queue_fill(queue_t* queue);



/* Clears the transition queue. Safe to call while a previously defined queue is
 * executing.
 */
ret_code_t queue_clear();



/* Starts executing the queue according to its contents. It is safe to use the
 * queue modification functions after calling this function. This function will
 * return an error if a queue is already executing and the current queue will not
 * be modified or cleared.
 */
ret_code_t queue_execute();



/* A shortcut, useful if you're managing your queue(s) externally to the driver
 * or if you need to write something without disturbing the normal queue.
 * Copies the provided queue into driver memory and immediately executes it
 * (respecting the same constraints as queue_execute). The normal internal
 * queue is not modified or cleared. The parameters are handled as in
 * queue_fill. The provided queue is safe to be overwritten or freed after
 * this function is called.
 * buffer: a pointer to an array of transitions
 * count: how many transitions (NOT bytes) are to be executed
 */
ret_code_t queue_execute_external(queue_t* queue);



/* Stops the currently executing transition queue. The current transition will
 * be finished. The queue cannot be resumed; to start another queue, fill it
 * with queue_add or queue_fill and call queue_execute.
 */
ret_code_t queue_stop();



/* Returns the executing/stopped state of the queue.
 */
ret_code_t queue_get_state();

#endif
