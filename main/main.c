#include <stdint.h>
#include <stdbool.h>

#include "esp_attr.h"
#include "FreeRTOS.h"
#include "task.h"

#include "display.h"
#include "graphics.h"

void app_main() {

  disp_driver_init(9);

  transition_t trans;
  trans.icon_delay = 0;
  trans.icon_delay = 33;
  trans.frame_delay = 100;
  trans.instant = false;
  //trans.instant = true;
  trans.space = 1;

  for( uint8_t i = FIRST_ICON; i <= LAST_ICON; i++ ) {
    trans.icon = icon[i];
    disp_queue_append_single( &trans );
  }

  while( true ) {
    disp_queue_start();
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
