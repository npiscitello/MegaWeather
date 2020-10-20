#include <stdint.h>
#include <stdbool.h>

#include "esp_attr.h"

#include "display.h"
#include "graphics.h"

void app_main() {

  driver_init(5);

  transition_t trans;
  trans.frame_delay = 75;
  trans.instant = false;
  trans.space = 2;

  trans.icon = digit[0];
  queue_add( &trans );

  trans.icon = digit[1];
  queue_add( &trans );

  trans.icon = digit[2];
  queue_add( &trans );

  trans.icon = digit[3];
  queue_add( &trans );

  trans.space = 3;
  trans.icon = icon[FOG];
  queue_add( &trans );

  // this tests adding too many things to the queue
  trans.icon = digit[4];
  queue_add( &trans );

  queue_execute();
}
