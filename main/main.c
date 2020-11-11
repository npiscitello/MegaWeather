#include <stdint.h>
#include <stdbool.h>

#include "esp_attr.h"
#include "FreeRTOS.h"
#include "task.h"

#include "display.h"
#include "graphics.h"

void app_main() {

  disp_driver_init(5);

  transition_t trans;
  trans.icon_delay = 0;
  trans.frame_delay = 75;
  trans.instant = false;
  //trans.instant = true;
  trans.space = 1;

  trans.icon = digit[0];
  disp_queue_append_single( &trans );

  trans.icon = digit[1];
  disp_queue_append_single( &trans );

  trans.icon = digit[2];
  disp_queue_append_single( &trans );

  trans.icon = digit[3];
  trans.icon_delay = 50;
  disp_queue_append_single( &trans );

  trans.space = 4;
  trans.icon = icon[FOG];
  disp_queue_append_single( &trans );

  // this tests adding too many things to the queue
  trans.icon = digit[5];
  disp_queue_append_single( &trans );

  while( true ) {
    disp_queue_start();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
