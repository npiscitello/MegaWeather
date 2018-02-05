#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "c_types.h"

// the struct defining an icon
struct icon_t {
  uint64_t icon;      // the actual bitmap icon, max 8x8
  uint8_t width;      // the number of columns required to fully display the icon
};
typedef struct icon_t icon_t;

// the arrays holding the actual data
extern const icon_t image_arr[];
extern const icon_t digit_arr[];
extern const icon_t character_arr[];

// direct links to specific icons
// comments represent the API "icon" strings each icon will correspond to
//      NAME    INDEX     TYPE      API "ICON" STRINGS
#define SUN       0   //  clear     (clear, sunny) (daytime)
#define MOON      1   //  clear     (clear, sunny) (nighttime)
#define P_CLOUD   2   //  clear     (partlycloudy, mostlysunny)
#define M_CLOUD   3   //  cloudy    (mostlycloudy, partlysunny)
#define CLOUD     4   //  cloudy    (cloudy)
#define PRECIP    5   //  precip    (chancerain, chancesleet, chancetstorms, sleet, rain, tstorms)
#define SNOW      6   //  precip    (chanceflurries, chancesnow, flurries, snow)
#define WIND      7   //  clear     (any clear/cloudy status and wind over a certain threshhold)
#define FOG       8   //  cloudy    (fog, hazy)
// Link to the first and last icons
#define FIRST_ICON     SUN
#define LAST_ICON      FOG

// direct links to specific characters
#define EXCLAIM   0
#define QUESTION  1   // unknown
#define BLANK     2
#define LOAD_0    3
#define LOAD_1    4
#define LOAD_2    5
#define NEGATIVE  6
#define DEGREE    7

#endif
