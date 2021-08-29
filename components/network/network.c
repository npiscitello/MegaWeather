#include <esp_wifi.h>

#include "network.h"

// connect to an AP
uint8_t connect() {
  // default setup, wifi station
  wifi_init_config_t network_config = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&network_config);

  wifi_config_t wifi_config = {
    .sta = {
      .ssid = "CastleSuperBeast",
      .password = ">}GW,h'}7zX9",
      .threshold = {
        .authmode = WIFI_AUTH_WPA2_PSK
      },
    },
  };
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
  esp_wifi_start();


  return 1;
}

// [TESTING ONLY] pull data from a website
uint8_t pull_data() {
  return 1;
}
