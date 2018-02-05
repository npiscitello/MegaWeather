#include <ets_sys.h>
#include <osapi.h>
#include <os_type.h>

#include "graphics.h"
#include "display.h"
#include "network.h"

// blink timers
static volatile os_timer_t change_timer;
#define PERIOD 1500

void ICACHE_FLASH_ATTR disp_image(void *arg) {
  (void)arg;
  execute_transition();
  //update_screen(image_arr[SNOW]);
}

void ICACHE_FLASH_ATTR user_init() {
  display_init();

  // setup timers
  os_timer_setfn(&change_timer, (os_timer_func_t *)disp_image, NULL);
  os_timer_arm(&change_timer, PERIOD, 1);
}
