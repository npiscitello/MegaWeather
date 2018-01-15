#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "graphics.h"
#include "display.h"


// blink timers
static volatile os_timer_t change_timer;
#define PERIOD 5000

void ICACHE_FLASH_ATTR disp_image(void *arg) {
  (void)arg;

  transition_t trans;
  trans.frame_delay = 100;

  trans.space = 3;
  trans.icon = digit[0];
  add_to_queue( &trans );

  trans.space = 1;
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

void ICACHE_FLASH_ATTR user_init()
{

  display_init();

  // setup timers
  os_timer_setfn(&change_timer, (os_timer_func_t *)disp_image, NULL);
  os_timer_arm(&change_timer, PERIOD, 1);

}
