#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"


/*
PIN_FUNC_SELECT(<COL_1>, FUNC_<COL_0>);
GPIO0:  PERIPHS_IO_MUX_GPIO0_U
GPIO1:  PERIPHS_IO_MUX_U0TXD_U
GPIO2:  PERIPHS_IO_MUX_GPIO2_U
GPIO3:  PERIPHS_IO_MUX_U0RXD_U
GPIO4:  PERIPHS_IO_MUX_GPIO4_U
GPIO5:  PERIPHS_IO_MUX_GPIO5_U
GPIO6:  PERIPHS_IO_MUX_SD_CLK_U
GPIO7:  PERIPHS_IO_MUX_SD_DATA0_U
GPIO8:  PERIPHS_IO_MUX_SD_DATA1_U
GPIO9:  PERIPHS_IO_MUX_SD_DATA2_U
GPIO10: PERIPHS_IO_MUX_SD_DATA3_U
GPIO11: PERIPHS_IO_MUX_SD_CMD_U
GPIO12: PERIPHS_IO_MUX_MTDI_U
GPIO13: PERIPHS_IO_MUX_MTCK_U
GPIO14: PERIPHS_IO_MUX_MTMS_U
GPIO15: PERIPHS_IO_MUX_MTDO_U
*/

// pin the LED is on
#define GPIO BIT5

// the LED is on for 50ms every 250ms
#define LEDPER 250
#define LEDON 50

static volatile os_timer_t on_timer;
static volatile os_timer_t off_timer;

void ICACHE_FLASH_ATTR led_off(void *arg) {
  gpio_output_set(GPIO, 0, 0, 0);
  os_timer_disarm(&off_timer);
}

void ICACHE_FLASH_ATTR led_on(void *arg) {
  gpio_output_set(0, GPIO, 0, 0);
  os_timer_arm(&off_timer, LEDON, 1);
}

/*
os_event_t    user_procTaskQueue[1];
void ICACHE_FLASH_ATTR loop(os_event_t *events) {
  gpio_output_set(0,GPIO,0,0);
  os_delay_us(50000);
  gpio_output_set(GPIO,0,0,0);
  os_delay_us(200000);
  system_os_post(0, 0, 0 );
}
*/

void ICACHE_FLASH_ATTR user_init()
{
  // init gpio subsytem
  gpio_init();

  // config GPIO5 to actually be a BPIO and enable output
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
  gpio_output_set(0, 0, GPIO, 0);
  gpio_output_set(GPIO, 0, 0, 0);

  // setup timers
  os_timer_setfn(&on_timer, (os_timer_func_t *)led_on, NULL);
  os_timer_setfn(&off_timer, (os_timer_func_t *)led_off, NULL);
  os_timer_arm(&on_timer, LEDPER, 1);

  //system_os_task(loop, 0, user_procTaskQueue, 1);

  //system_os_post(0, 0, 0 );
}
