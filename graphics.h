#ifndef GRAPHICS_H
#define GRAPHICS_H

// weather icons as noted - some stolen shamelessly from
// https://electricimp.com/docs/learning/weather/
// YCM will complain about this - it uses a GCC feature not implemented in clang
const __flash uint64_t icon[] =
{
  0x3c4299bdbd99423c,   // clear (sun)
  0x30180c0e0e0c1830,   // clear (moon)
  0x0000007e8181621c,   // partly cloudy
  0x0000007effff7e1c,   // cloudy
  0x8452087effff7e1c,   // precip (rain, sleet, etc.)
  0xa524e71818e724a5,   // snow
  0x7e01e61060fc020c,   // wind
  0xaa55aa55aa55aa55    // fog
};

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
#define FIRST     SUN
#define LAST      FOG

// YCM will complain about this - it uses a GCC feature not implemented in clang
const __flash uint64_t digit[] =
{
  0x0e1111111111110e,   // 0
  0x1f04040404050604,   // 1
  0x1f0204081010110e,   // 2
  0x0e1110100c10110e,   // 3
  0x10101f1112141810,   // 4
  0x0e1110100f01011f,   // 5
  0x0e1111110f01110e,   // 6
  0x020202040810101f,   // 7
  0x0e1111110e11110e,   // 8
  0x0e11101e1111110e    // 9
};

// YCM will complain about this - it uses a GCC feature not implemented in clang
const __flash uint64_t character[] =
{
  0x00180018183c3c18,   // !
  0x001800183060663c,   // ?
  0x0000000000000000,   // 
  0x001800183060663c,   // .
  0x0000001b1b000000,   // ..
  0x000000dbdb000000,   // ...
  0x0000000000000303    // degrees
};

// direct links to specific characters
#define EXCLAIM   0
#define QUESTION  1
#define BLANK     2
#define LOAD_0    3
#define LOAD_1    4
#define LOAD_2    5
#define DEGREE    6

#endif
