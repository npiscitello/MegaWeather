#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "graphics.h"
#include "display.h"
#include "network.h"
#include "user_interface.h"


// blink timers
static volatile os_timer_t change_timer;
//#define PERIOD 2100
#define PERIOD 2750

void ICACHE_FLASH_ATTR disp_image(void *arg) {
  (void)arg;

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

void ICACHE_FLASH_ATTR user_init()
{

  display_init();

  // setup timers
  os_timer_setfn(&change_timer, (os_timer_func_t *)disp_image, NULL);
  os_timer_arm(&change_timer, PERIOD, 1);

}

// required by SDK
uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 8;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}
