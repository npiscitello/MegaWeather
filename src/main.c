#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "graphics.h"
#include "display.h"


// blink timers
static volatile os_timer_t change_timer;
#define PERIOD 1500

uint8_t counter = 0;

void ICACHE_FLASH_ATTR disp_image(void *arg) {
  (void)arg;

  transition_data_t trans;
  trans.icon = digit[counter];
  trans.space = 1;
  trans.frame_delay = 50;

  add_to_queue( &trans );
  execute_queue();

  if( counter++ == 9 ) {
    counter = 0;
  }

  /*
  if( !transition_running() ) {
    if( counter <= 9 ) {
      transition(digit[counter], 1, 100);
    } else {
      transition(icon[FOG], 1, 100);
    }
    
    if( counter++ == 10 ) {
      counter = 0;
    }
  }
  */
}

void ICACHE_FLASH_ATTR user_init()
{

  display_init();

  // setup timers
  os_timer_setfn(&change_timer, (os_timer_func_t *)disp_image, NULL);
  os_timer_arm(&change_timer, PERIOD, 1);

}
