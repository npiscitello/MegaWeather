#include <stdio.h>

#include "display.h"
#include "graphics.h"

void app_main() {
  // init spi
  printf("[main] pre display_init\n");
  display_init();
  // push something to the screen
  printf("[main] post display_init, pre update_screen\n");
  update_screen(character[QUESTION]);
  // ...that's all folks!
  printf("[main] post update_screen\n");
}

/*

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_spi_flash.h"

void app_main()
{
    printf("Hello world!\n");

    // Print chip information
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);
    printf("This is ESP8266 chip with %d CPU cores, WiFi, ",
            chip_info.cores);

    printf("silicon revision %d, ", chip_info.revision);

    printf("%dMB %s flash\n", (int)spi_flash_get_chip_size() / (1024 * 1024),
            (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    for (int i = 10; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
*/

/*
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
*/
