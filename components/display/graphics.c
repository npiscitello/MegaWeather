#define DISABLED

#ifdef DISABLED
#else

#include "graphics.h"

// weather icons as noted - some stolen shamelessly from
// https://electricimp.com/docs/learning/weather/
const icon_t ICACHE_RODATA_ATTR icon[] =
{
  {0x3c4299bdbd99423c, 8},   // clear (sun)
  {0x30180c0e0e0c1830, 8},   // clear (moon)
  {0x0000007e8181621c, 8},   // partly cloudy
  {0x0000007ed5ab761c, 8},   // mostly cloudy
  {0x0000007effff7e1c, 8},   // cloudy
  {0x8452087effff7e1c, 8},   // precip (rain, sleet, etc.)
  {0xa524e71818e724a5, 8},   // snow
  {0x7e01e61060fc020c, 8},   // wind
  {0xaa55aa55aa55aa55, 8}    // fog
};

const icon_t ICACHE_RODATA_ATTR digit[] =
{
  {0x0e1111111111110e, 5},   // 0
  {0x1f04040404050604, 5},   // 1
  {0x1f0204081010110e, 5},   // 2
  {0x0e1110100c10110e, 5},   // 3
  {0x10101f1112141810, 5},   // 4
  {0x0e1110100f01011f, 5},   // 5
  {0x0e1111110f01110e, 5},   // 6
  {0x020202040810101f, 5},   // 7
  {0x0e1111110e11110e, 5},   // 8
  {0x0e11101e1111110e, 5}    // 9
};

const icon_t ICACHE_RODATA_ATTR character[] =
{
  {0x00180018183c3c18, 8},   // !
  {0x001800183060663c, 8},   // ?
  {0x0000000000000000, 8},   // 
  {0x001800183060663c, 8},   // .
  {0x0000001b1b000000, 8},   // ..
  {0x000000dbdb000000, 8},   // ...
  {0x0000000f00000000, 4},   // -
  {0x0000000000000303, 8}    // degrees
};

#endif
