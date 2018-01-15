#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "c_types.h"

// the struct defining an icon
struct icon_t {
  uint64_t icon;      // the actual bitmap icon, max 8x8
  uint8_t width;      // the number of columns required to fully display the icon
};
typedef struct icon_t icon_t;

extern const icon_t icon[];
// direct links to specific icons
#define SUN       0
#define MOON      1
#define P_CLOUD   2
#define CLOUD     3
#define PRECIP    4
#define SNOW      5
#define WIND      6
#define FOG       7
// Link to the first and last icons
#define FIRST_ICON     SUN
#define LAST_ICON      FOG

extern const icon_t digit[];

extern const icon_t character[];
// direct links to specific characters
#define EXCLAIM   0
#define QUESTION  1
#define BLANK     2
#define LOAD_0    3
#define LOAD_1    4
#define LOAD_2    5
#define NEGATIVE  6
#define DEGREE    7

#endif
