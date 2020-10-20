#include <stdint.h>
#include <stdbool.h>

#include "esp_attr.h"

#include "display.h"
#include "graphics.h"

void app_main() {

  display_init();

  transition_t trans;
  trans.frame_delay = 75;
  trans.instant = false;
  trans.space = 1;

  trans.icon = digit[0];
  add_to_queue( &trans );

  trans.icon = digit[1];
  add_to_queue( &trans );

  trans.icon = digit[2];
  add_to_queue( &trans );

  trans.icon = digit[3];
  add_to_queue( &trans );

  trans.space = 3;
  trans.icon = icon[FOG];
  add_to_queue( &trans );

  // this tests adding too many things to the queue
  trans.icon = digit[4];
  add_to_queue( &trans );

  execute_queue();
}
