#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "esp_attr.h"
#include "driver/spi.h"

#include "display.h"
#include "graphics.h"

// <DEBUG>
#define TAG "mw"

// platform-specific
#define ULONG_MAX 0xFFFFFFFF

// actual pixel dimensions of the LED array
#define SCREEN_HEIGHT 8
#define SCREEN_WIDTH 8

// Stores the current state of the screen. This could be just a uint64_t, but
// using the icon struct allows us to easily change the way icon storage is
// implemented in graphics.h if needed.
icon_t cur_screen = {0, SCREEN_WIDTH};

// storage for the internal queue
queue_t queue;

// We only want one thing to be able to mess with the queue at a time, otherwise
// it may get corrupted.
SemaphoreHandle_t queue_mutex = NULL;

// handle for the write_to_display task
TaskHandle_t queue_execute_task = NULL;

// things we can tell the queue execution task to do
enum queue_execute_cmds {
  QUEUE_CMD_NOP   = 0,
  QUEUE_CMD_START = 1,
  QUEUE_CMD_STOP  = 2
};



/* HERE BE CUTE BABY DRAGONS
 * These are the user-facing wrapper functions. Contains all the driver logic,
 * which is pretty much just queue management. No hardware manangement goes on
 * here; that stuff is all below.
 */

ret_code_t disp_queue_append_single(transition_t* item) {
  ret_code_t retcode = RET_NO_ERR;
  if( xSemaphoreTake(queue_mutex, 0) == pdTRUE ) {
    if( queue.length < queue.max_length ) {
      queue.ptr[queue.length] = *item;
      queue.length++;
    } else {
      retcode = RET_QUEUE_FULL;
    }
    xSemaphoreGive(queue_mutex);
  } else {
    retcode = RET_QUEUE_LOCKED;
  }
  return retcode;
}



ret_code_t disp_queue_clear_single() {
  ret_code_t retcode = RET_NO_ERR;
  if( xSemaphoreTake(queue_mutex, 0) == pdTRUE ) {
    if( queue.length > 0 ) {
      queue.length--;
    }
    xSemaphoreGive(queue_mutex);
  } else {
    return RET_QUEUE_LOCKED;
  }
  return retcode;
}



ret_code_t disp_queue_append(queue_t* ext_queue) {
  ret_code_t retcode = RET_NO_ERR;
  if( xSemaphoreTake(queue_mutex, 0) == pdTRUE ) {
    uint8_t trans_count = ext_queue->length;
    if( queue.length + trans_count > queue.max_length) {
      trans_count = queue.max_length - queue.length;
      retcode = RET_QUEUE_TRUNC;
    }
    memcpy(&(queue.ptr[queue.length]), ext_queue->ptr, 
        trans_count * sizeof(transition_t));
    xSemaphoreGive(queue_mutex);
  } else {
    retcode = RET_QUEUE_LOCKED;
  }
  return retcode;
}



ret_code_t disp_queue_clear() {
  ret_code_t retcode = RET_NO_ERR;
  if( xSemaphoreTake(queue_mutex, 0) == pdTRUE ) {
    // we don't need to actually erase bytes, just make the queue think there's
    // nothing in it!
    queue.length = 0;
    xSemaphoreGive(queue_mutex);
  } else {
    retcode = RET_QUEUE_LOCKED;
  }
  return retcode;
}



ret_code_t disp_queue_reset() {
  ret_code_t retcode = RET_NO_ERR;
  if( xSemaphoreTake(queue_mutex, 0) == pdTRUE ) {
    queue.index = 0;
    xSemaphoreGive(queue_mutex);
  } else {
    retcode = RET_QUEUE_LOCKED;
  }
  return retcode;
}


ret_code_t disp_queue_start() {
  xTaskNotify(queue_execute_task, QUEUE_CMD_START, eSetValueWithOverwrite);
  return RET_NO_ERR;
}



ret_code_t disp_queue_stop() {
  xTaskNotify(queue_execute_task, QUEUE_CMD_STOP, eSetValueWithOverwrite);
  return RET_NO_ERR;
}






/* HERE BE SCARY ADULT DRAGONS
 * Functions below this line are low-level actual real-life hardware and OS
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
// <TODO> catch any errors from spi_transmit
ret_code_t ICACHE_FLASH_ATTR disp_set_icon( const icon_t image ) {
  // each transmit sets a full row
  for( uint8_t row = 0x01; row <= 0x08; row++ ) {
    // this could probably be abstracted away a bit; for now, we know we're
    // using uint64_t to simulate an 8 member uint8_t array, so we'll keep this
    // magic number for now...
    spi_transmit(row, (uint8_t)(image.icon >> ((row - 1) * 8)));
  }
  cur_screen = image;
  return RET_NO_ERR;
}



// change the software-defined display brightness
ret_code_t ICACHE_FLASH_ATTR disp_set_brightness( uint8_t brightness ) {
  return spi_transmit(0x0A, brightness);
}



// the task function that actually writes to the display
void disp_queue_execute(void* arg) {
  while(1) {
    uint32_t cmd;
    xTaskNotifyWait(0, ULONG_MAX, &cmd, portMAX_DELAY);
    if( cmd == QUEUE_CMD_START ) {
      cmd = QUEUE_CMD_NOP;
      // doesn't matter if this blocks (separate task from user code)
      xSemaphoreTake(queue_mutex, portMAX_DELAY);
      // if the index is out of bounds, start from the beginning
      queue.index = (queue.index >= queue.length) ? 0 : queue.index;
      while(true) {
        // this loop cycles once per transition in queue.ptr - we don't want
        // the user to be able to interrupt a write mid-transition
        if( queue.ptr[queue.index].instant == true ) {
          // instant update, if requested...
          disp_set_icon(queue.ptr[queue.index].icon);

        } else {
          // ...otherwise, do an animated update
        }

        cur_screen = queue.ptr[queue.index].icon;
        queue.index++;

        // should we stop? (this also implements the inter-frame wait)
        xTaskNotifyWait(0, ULONG_MAX, &cmd, 
            pdMS_TO_TICKS(queue.ptr[queue.index].icon_delay * 10));
        if( cmd == QUEUE_CMD_STOP || queue.index >= queue.length ) { 
          cmd = QUEUE_CMD_NOP;
          break; 
        }
      }
      xSemaphoreGive(queue_mutex);
    }
  }
}

/*
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



// for some reason, SPI won't work without a callback defined
void spi_event_callback(int event, void *arg) {};

// set up the screen and other variables for first use
// <TODO> grab any errors from the SPI writes or memory allocations
ret_code_t ICACHE_FLASH_ATTR disp_driver_init( const uint8_t queue_size ) {
  // give the queue some space to breathe
  queue.ptr = malloc(queue_size * sizeof(transition_t));
  queue.length = 0;
  queue.max_length = queue_size;
  queue.index = 0;

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
  // for some reason, SPI won't work without a callback defined
  spi_config_data.intr_enable.val = 0;
  spi_config_data.event_cb = spi_event_callback;
  spi_config_data.mode = SPI_MASTER_MODE;
  spi_config_data.clk_div = SPI_2MHz_DIV;

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
  disp_set_brightness(0x08);
  // scan across all digits
  spi_transmit(0x0b, 0x07);
  // turn off all pixels
  disp_set_icon(character[BLANK]);
  // take the chip out of shutdown
  spi_transmit(0x0C, 0x01);

  // set up the write to display task and the execution queue mutex
  // <TODO> Should the stack be smaller? bigger?
  xTaskCreate(disp_queue_execute, "queue_execute", 2048, NULL,
      DISPLAY_PRIORITY, &queue_execute_task);

  queue_mutex = xSemaphoreCreateMutex();

  return RET_NO_ERR;
}
