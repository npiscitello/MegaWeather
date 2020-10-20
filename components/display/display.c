#include <stdlib.h>
#include <string.h>

#include "esp_attr.h"
#include "driver/spi.h"

#include "display.h"
#include "graphics.h"

// actual pixel dimensions of the LED array
#define SCREEN_HEIGHT 8
#define SCREEN_WIDTH 8

// global variables are gross, but this one is necessary - stores the current
// state of the screen otherwise, the user would have to keep track of what's
// on the screen manually
// uses a bit of memory, but I think we can spare it for convenience 
// this could be just a uint64_t, but using the icon struct allows us to easily
// change the way icon storage is implemented in graphics.h if needed
icon_t cur_screen = {0, 8};

// the storage for both queues - user-facing and currently executing
queue_t user_queue;       // user-facing; never gets executed directly. Always
                          // safe to be messed with.
queue_t execution_queue;  // internal-only, so the user can mess with the user
                          // queue without disturbing what's being written to
                          // the screen. DON'T TOUCH while executing!

// janky mutexes
struct mutex {
  volatile uint8_t screen_write  :1; // true when the screen is being written
  volatile uint8_t trans_exec    :1; // true when there's a transition happening
  volatile uint8_t queue_exec    :1; // true when the queue is executing
  volatile uint8_t mutex4        :1; 
  volatile uint8_t mutex3        :1;
  volatile uint8_t mutex2        :1;
  volatile uint8_t mutex1        :1;
  volatile uint8_t mutex0        :1;
} mutex;



/* HERE BE CUTE BABY DRAGONS
 * These are the user-facing wrapper functions. Contains pretty much all the
 * driver logic, which is mostly just invoking the low-level stuff in the right
 * order and at the right times. No memory or hardware mamangement goes on
 * here; that stuff is all below.
 */

// baby dragons need the support of mama dragons
ret_code_t copy_queue_data(queue_t* dst, queue_t* src);
ret_code_t append_queue_data(queue_t* dst, queue_t* src);
void erase_queue_mem(queue_t* queue);
ret_code_t write_queue_to_display(queue_t* queue);

ret_code_t queue_fill(queue_t* ext_queue) {
  return append_queue_data(&user_queue, ext_queue);
}

ret_code_t queue_clear() {
  // screw you, user queue
  erase_queue_mem(&user_queue);
  return RET_NO_ERR;
}

// I was debating if this should be exposed or not, but having a single queue is
// pretty limiting. If the user wants to manage their own queue, all power to
// 'em!
ret_code_t queue_execute_external(queue_t* ext_queue) {
  // if the queue is executing, messing with the execution queue is a Bad Thing
  ret_code_t queue_state = queue_get_state();
  if( queue_state == RET_QUEUE_STOPPED ) {
    ret_code_t ret_cpy = copy_queue_data(&execution_queue, ext_queue);
    ret_code_t ret_exec = write_queue_to_display(&execution_queue);
    // if the exec failed, we don't care if the queue was truncated
    return (ret_exec == RET_NO_ERR) ? ret_cpy : ret_exec;
  }
  return queue_state;
}

ret_code_t queue_execute() {
  return queue_execute_external(&user_queue);
}

ret_code_t queue_stop() {
  // we abuse the fact that the low-level queue write function isn't pure: by
  // setting the execution queue's length to zero, when it's time to load the
  // next transition, it'll look like we're done! As a bonus, the functions
  // clean up after themselves like normal!

  // if this doesn't work, you may need to make the length member volatile
  execution_queue.length = 0;
  return RET_NO_ERR;
}

ret_code_t queue_get_state() {
  // I do love the smell of a simple wrapper in the morning
  return mutex.queue_exec ? RET_QUEUE_EXECUTING : RET_QUEUE_STOPPED;
}



/* HERE BE MOODY TEENAGE DRAGONS
 * These functions are responsible for scheduling and leverage the ESP RTOS.
 * You'll DEFINITELY need to mess with these if you're porting this to another
 * platform.
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



/* HERE BE SCARY ADULT DRAGONS
 * Functions below this line are low-level actual real-life hardware and memory
 * management functions. There's no logic down here, so you probably won't need
 * to mess with these unless you're porting this to a new platform/display
 * device.
 */

// transmit a register/data pair
// <TODO> catch the spi_trans return code to feed into the custom retcode
ret_code_t ICACHE_FLASH_ATTR spi_transmit(uint8_t addr, uint8_t data) {
  spi_trans_t spi_packet;

  // The addr reg likes to send the top byte first and the bottom byte last,
  // meaning we'd have to shift 8 bit values 24 places left. That's annoying,
  // so we're using the cmd reg instead.
  spi_packet.bits.cmd = sizeof(addr) * 8;
  spi_packet.cmd = (uint16_t*)&addr;
  spi_packet.bits.mosi = sizeof(data) * 8;
  spi_packet.mosi = (uint32_t*)&data;
  spi_packet.bits.addr = 0;
  spi_packet.bits.miso = 0;

  spi_trans(HSPI_HOST, &spi_packet);

  return RET_NO_ERR;
}

// update the whole screen in one shot
// I'm not worrying about mutex locking...yet. I'll figure something out if it becomes a problem.
// <TODO> catch any errors from spi_transmit
ret_code_t ICACHE_FLASH_ATTR display_update( const icon_t image ) {
  for( uint8_t i = 0x01; i <= 0x08; i++ ) {
    // this could probably be abstracted away a bit; for now, we know we're using uint64_t to
    // simulate an 8 member uint8_t array, so we'll keep this magic number for now...
    spi_transmit(i, (uint8_t)(image.icon >> ((i - 1) * 8)));
  }
  cur_screen = image;
  return RET_NO_ERR;
}

// change the software-defined display brightness
ret_code_t ICACHE_FLASH_ATTR display_set_bright( uint8_t brightness ) {
  return spi_transmit(0x0A, brightness);
}

// overwrite dst with src
ret_code_t copy_queue_data(queue_t* dst, queue_t* src) {
  ret_code_t retcode = RET_NO_ERR;
  uint8_t trans_count = src->length;
  if( trans_count > dst->max_length ) {
    trans_count = dst->max_length;
    retcode = RET_QUEUE_TRUNC;
  }
  memcpy(dst->ptr, src->ptr, trans_count * sizeof(transition_t));
  return retcode;
}

// append as much of src as possible to dst
ret_code_t append_queue_data(queue_t* dst, queue_t* src) {
  ret_code_t retcode = RET_NO_ERR;
  uint8_t trans_count = src->length;
  if( dst->length + trans_count > dst->max_length) {
    trans_count = dst->max_length - dst->length;
    retcode = RET_QUEUE_TRUNC;
  }
  memcpy(&(dst->ptr[dst->length]), src->ptr, 
      trans_count * sizeof(transition_t));
  return retcode;
}

// immediately set all the memory in a queue to zero
void erase_queue_mem(queue_t* queue) {
  memset(queue, 0x00, sizeof(queue_t));
}

// run timers and stuff to shift the queue out to the display
// this function is, by design, NOT pure! If you mess with or depend on the
// buffer that is passed to this, bad things will happen!
ret_code_t write_queue_to_display(queue_t* queue) {
  // make the LEDs light up with, uh, magic or something...
  // we're done with the queue - burn it to the ground!
  erase_queue_mem(queue);
  return RET_NO_ERR;
}

// set up the screen and other variables for first use
// <TODO> grab any errors from the SPI writes or memory allocations
ret_code_t ICACHE_FLASH_ATTR driver_init( const uint8_t queue_size ) {
  user_queue.ptr = malloc(queue_size * sizeof(transition_t));
  user_queue.length = 0;
  user_queue.max_length = queue_size; 
  user_queue.index = 0;

  execution_queue.ptr = malloc(queue_size * sizeof(transition_t));
  execution_queue.length = 0;
  execution_queue.max_length = queue_size;
  execution_queue.index = 0;

  // configure SPI
  spi_config_t spi_config_data;
  spi_config_data.interface.cs_en = 1;
  spi_config_data.interface.mosi_en = 1;
  spi_config_data.interface.miso_en = 0;
  // not clear what this does, even after exploring with a logic analyzer
  spi_config_data.interface.byte_tx_order = 0;
  // endianness - 0 means bytes are written out MSB first, LSB last.
  spi_config_data.interface.bit_tx_order = 0;
  spi_config_data.interface.cpha = 0;
  spi_config_data.interface.cpol = 0;
  // individual SPI events give us low level info we don't need
  spi_config_data.intr_enable.val = 0;
  spi_config_data.mode = SPI_MASTER_MODE;
  // gotta go... fAST
  spi_config_data.clk_div = SPI_10MHz_DIV;

  // use the external (not-flash) pins
  spi_init(HSPI_HOST, &spi_config_data);

  // setup for the MAX7221 chip (through a TXB0104 level shifter)
  // don't assume we're on a clean startup of the chip
  // put the chip into shutdown
  spi_transmit(0x0C, 0x00);
  // take out of display test mode
  spi_transmit(0x0F, 0x00);
  // don't use the decode table
  spi_transmit(0x09, 0x00);
  // set intensity to middle ground
  display_set_bright(0x08);
  // scan across all digits
  spi_transmit(0x0b, 0x07);
  // turn off all pixels
  display_update(character[BLANK]);
  // take the chip out of shutdown
  spi_transmit(0x0C, 0x01);

  return RET_NO_ERR;
}

// <TODO> this is old stuff - delete when everything implemented
/*
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
