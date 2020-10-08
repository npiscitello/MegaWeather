#ifndef NETWORK_H
#define NETWORK_H

#include <c_types.h>

#define ICON_LEN 32

struct weather_data {
  char icon[ICON_LEN];
  uint8_t feelslike_f;
  uint8_t wind_mph;
};
typedef struct weather_data weather_data_t;


/* connect to a WiFi AP
 * ssid: pointer to a character array containing the network name. Passed directly to memcpy.
 * ssid_len: length of the name; don't forget the null terminator! Passed directly to memcpy.
 * password: pointer to a character array containing the network password. Passed directly to memcpy.
 * password_len: length of the password; don't forget the null terminator! Passed directly to memcpy.
 */
void connect( const char* ssid, 
              const uint8_t ssid_len,
              const char* password,
              const uint8_t password_len);

/* hit the WUnderground API to update conditions
 * data_struct: a pointer to the weather_data struct for the callback to fill
 */
void update_data( weather_data_t* data_struct );

#endif
