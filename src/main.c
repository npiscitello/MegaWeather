#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "driver/uart.h"
#include "user_interface.h"
#include "espconn.h"

// UART baud rate
#define BAUD BIT_RATE_115200

// pin the LED is on
#define GPIO BIT5

// the LED is on for 50ms every 1000ms
#define LEDPER 1000
#define LEDON 50

#define SSID "hackwitus"
#define PASSWORD "hackWITus2017!"

struct espconn example_conn;
ip_addr_t example_ip;
esp_tcp example_tcp;

char example_host[] = "example.com";
char buffer[2048];

/*
static volatile os_timer_t on_timer;
static volatile os_timer_t off_timer;
*/

/*
void ICACHE_FLASH_ATTR led_off( void *arg ) {
  gpio_output_set(GPIO, 0, 0, 0);
  os_timer_disarm(&off_timer);
}

void ICACHE_FLASH_ATTR led_on( void *arg ) {
  gpio_output_set(0, GPIO, 0, 0);
  os_timer_arm(&off_timer, LEDON, 1);
}
*/

void ICACHE_FLASH_ATTR data_received( void *arg, char *pdata, unsigned short len ) {
  struct espconn *conn = arg;
  os_printf( "%s: %s\n", __FUNCTION__, pdata );
  espconn_disconnect( conn );
}

void ICACHE_FLASH_ATTR tcp_connected( void *arg ) {
  struct espconn* conn = arg;
  espconn_regist_recvcb( conn, data_received );
  char buffer[] = "GET / HTTP/1.1\r\nHost: www.example.com\r\nAccept: */*\r\n\r\n";
  espconn_sent( conn, buffer, os_strlen(buffer));
}


void ICACHE_FLASH_ATTR dns_found( const char* name, ip_addr_t *ipaddr, void *arg ) {
  struct espconn *conn = arg;

  if( ipaddr == NULL ) {
    os_printf("DNS Failed!");
  } else {
    os_printf("connecting...");

    conn->type = ESPCONN_TCP;
    conn->state = ESPCONN_NONE;
    conn->proto.tcp = &example_tcp;
    conn->proto.tcp->local_port = espconn_port();
    conn->proto.tcp->remote_port = 80;
    os_memcpy( conn->proto.tcp->remote_ip, &ipaddr->addr, 4 );

    espconn_regist_connectcb( conn, tcp_connected );
    espconn_connect( conn );
  }
}

void ICACHE_FLASH_ATTR wifi_callback( System_Event_t *evt ) {
  os_printf( "[%s: %d] ", __FUNCTION__, evt->event );

  switch( evt->event ) {
    case EVENT_STAMODE_CONNECTED:
      os_printf("connected!");
      break;

    case EVENT_STAMODE_DISCONNECTED:
      os_printf("disconnected...");
      break;

    case EVENT_STAMODE_GOT_IP:
      //os_printf("ip: %s", IP2STR(&evt->event_info.got_ip.ip));
      espconn_gethostbyname(&example_conn, example_host, &example_ip, dns_found);
      break;
  }

  os_printf("\n");
}

void ICACHE_FLASH_ATTR user_init() {

  // init debug UART
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, 1);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, 1);

  // baud rates
  uart_init(BAUD, BAUD);

  /*
  // init gpio subsytem
  gpio_init();

  // config GPIO5 to actually be a BPIO and enable output
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO5_U, FUNC_GPIO5);
  gpio_output_set(0, 0, GPIO, 0);
  gpio_output_set(GPIO, 0, 0, 0);

  // setup LED timers
  os_timer_setfn(&on_timer, (os_timer_func_t *)led_on, NULL);
  os_timer_setfn(&off_timer, (os_timer_func_t *)led_off, NULL);
  os_timer_arm(&on_timer, LEDPER, 1);
  */

  // setup wifi
  wifi_set_opmode(STATION_MODE);
  static struct station_config config;

  config.bssid_set = 0;
  os_memcpy(&config.ssid, SSID, 32);
  os_memcpy(&config.password, PASSWORD, 64);
  wifi_station_set_config(&config);

  wifi_set_event_handler_cb(wifi_callback);
}
