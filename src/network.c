#include "network.h"

#include "osapi.h"
#include "user_interface.h"

// ugh, more globals...
struct station_config stationConf;

void ICACHE_FLASH_ATTR connect( const char* ssid, const uint8_t ssid_len,
                                const char* password, const uint8_t password_len ) {
  wifi_set_opmode(STATION_MODE);
  os_memcpy(&stationConf.ssid, ssid, ssid_len);
  os_memcpy(&stationConf.password, password, password_len);
  wifi_station_set_config(&stationConf);
}



void ICACHE_FLASH_ATTR update_data( weather_data_t* data_struct ) {
// API request: conditions/hourly/q/MA/Boston.json

}
