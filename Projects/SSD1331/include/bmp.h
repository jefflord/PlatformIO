#include <Arduino.h>

#define FRAME_DELAY (42)
#define FRAME_WIDTH (32)
#define FRAME_HEIGHT (32)
#define FRAME_COUNT (sizeof(frames) / sizeof(frames[0]))

const byte PROGMEM frames[][128] = {
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,14,0,0,0,27,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,192,0,0,17,254,0,0,17,51,192,0,17,51,224,0,17,19,32,14,16,2,32,9,16,0,32,24,144,0,32,8,112,0,32,4,48,0,32,2,16,0,32,1,0,0,32,0,128,0,32,0,64,0,32,0,32,0,32,0,16,0,96,0,12,0,192,0,7,131,128,0,0,254,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,14,0,0,0,27,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,192,0,0,17,254,0,0,17,51,192,0,17,51,224,0,17,19,32,14,16,2,32,9,16,0,32,24,144,0,32,8,112,0,32,4,48,0,32,2,16,0,32,1,0,0,32,0,128,0,32,0,64,0,32,0,32,0,32,0,16,0,96,0,12,0,192,0,7,131,128,0,0,254,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,14,0,0,0,27,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,192,0,0,17,254,0,0,17,51,192,0,17,51,224,0,17,19,32,14,16,2,32,9,16,0,32,24,144,0,32,8,112,0,32,4,48,0,32,2,16,0,32,1,0,0,32,0,128,0,32,0,64,0,32,0,32,0,32,0,16,0,96,0,12,0,192,0,7,131,128,0,0,254,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,14,0,0,0,27,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,192,0,0,17,254,0,0,17,51,192,0,17,51,224,0,17,19,32,14,16,2,32,9,16,0,32,24,144,0,32,8,112,0,32,4,48,0,32,2,16,0,32,1,0,0,32,0,128,0,32,0,64,0,32,0,32,0,32,0,16,0,96,0,12,0,192,0,7,131,128,0,0,254,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,14,0,0,0,27,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,192,0,0,17,254,0,0,17,51,192,0,17,51,224,0,17,19,32,14,16,2,32,9,16,0,32,24,144,0,32,8,112,0,32,4,48,0,32,2,16,0,32,1,0,0,32,0,128,0,32,0,64,0,32,0,32,0,32,0,16,0,96,0,12,0,192,0,7,131,128,0,0,254,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,14,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,192,0,0,17,254,0,0,17,51,192,0,17,51,224,0,17,19,32,14,16,2,32,9,16,0,32,24,144,0,32,8,112,0,32,4,48,0,32,2,16,0,32,1,0,0,32,0,128,0,32,0,64,0,32,0,32,0,32,0,16,0,96,0,12,0,192,0,7,131,128,0,0,254,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,14,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,192,0,0,17,254,0,0,17,51,192,0,17,51,224,0,17,19,32,14,16,2,32,9,16,0,32,24,144,0,32,8,112,0,32,4,48,0,32,2,16,0,32,1,0,0,32,0,128,0,32,0,64,0,32,0,32,0,32,0,16,0,96,0,12,0,192,0,7,131,128,0,0,254,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,12,0,0,0,26,0,0,0,19,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,64,0,0,17,254,0,0,17,51,192,0,17,51,96,0,17,19,32,14,16,2,32,9,16,0,32,24,144,0,32,8,112,0,32,4,48,0,32,2,16,0,32,1,0,0,32,0,128,0,32,0,64,0,32,0,32,0,32,0,16,0,96,0,12,0,192,0,7,255,128,0,0,124,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,30,0,0,0,27,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,64,0,0,17,252,0,0,17,51,192,0,17,51,96,0,17,19,32,15,16,2,32,9,144,0,32,24,208,0,32,8,112,0,32,4,48,0,32,2,16,0,32,1,0,0,32,0,128,0,32,0,64,0,32,0,32,0,32,0,16,0,96,0,12,0,192,0,7,255,128,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,14,0,0,0,49,128,0,0,32,128,0,0,78,192,0,0,90,128,0,0,113,128,0,0,49,128,0,0,17,0,0,0,17,0,0,0,17,64,0,0,17,252,0,0,17,51,192,0,17,51,96,0,17,19,32,15,16,2,32,9,144,0,32,8,208,0,32,8,112,0,32,4,48,0,32,2,16,0,32,1,0,0,32,0,128,0,32,0,64,0,32,0,32,0,32,0,24,0,64,0,12,0,192,0,7,255,128,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,31,0,0,0,32,128,0,0,64,64,0,0,192,64,0,0,128,96,0,0,142,32,0,0,145,96,0,0,81,64,0,0,113,128,0,0,17,0,0,0,17,64,0,0,17,252,0,0,17,51,192,0,17,51,96,0,17,19,32,15,16,2,32,9,144,0,32,8,208,0,32,8,112,0,32,4,48,0,32,2,16,0,32,1,0,0,32,0,128,0,32,0,64,0,32,0,32,0,32,0,24,0,64,0,12,0,192,0,3,255,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,6,0,0,0,59,128,0,0,64,64,0,0,192,96,0,0,128,32,0,0,142,32,0,0,154,32,0,0,145,32,0,0,145,32,0,0,81,64,0,0,49,128,0,0,17,64,0,0,17,252,0,0,17,51,192,0,17,51,96,0,17,19,32,15,16,2,32,9,144,0,32,8,208,0,32,8,112,0,32,4,48,0,32,2,16,0,32,1,0,0,32,0,128,0,32,0,64,0,32,0,32,0,32,0,24,0,64,0,12,0,192,0,7,255,128,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,31,0,0,0,96,192,0,0,192,64,0,0,128,32,0,0,14,32,0,1,26,16,0,1,17,16,0,1,17,0,0,0,145,32,0,0,145,96,0,0,81,192,0,0,49,192,0,0,17,252,0,0,17,51,192,0,17,51,96,0,17,19,32,15,16,2,32,9,144,0,32,24,208,0,32,8,112,0,32,4,48,0,32,2,16,0,32,1,0,0,32,0,128,0,32,0,64,0,32,0,32,0,32,0,16,0,96,0,12,0,192,0,7,255,128,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,63,128,0,0,96,192,0,0,128,32,0,0,140,32,0,1,26,16,0,1,17,16,0,1,17,16,0,1,17,16,0,1,17,16,0,0,145,32,0,0,81,64,0,0,49,192,0,0,17,254,0,0,17,51,192,0,17,51,96,0,17,19,32,14,16,2,32,9,16,0,32,24,144,0,32,8,112,0,32,4,48,0,32,2,16,0,32,1,0,0,32,0,128,0,32,0,64,0,32,0,32,0,32,0,16,0,96,0,12,0,192,0,7,255,128,0,0,124,0,0,0,0,0},
  {0,0,0,0,0,49,128,0,0,64,64,0,0,128,32,0,1,14,16,0,1,17,16,0,1,17,16,0,1,17,16,0,1,17,16,0,1,17,16,0,0,145,32,0,0,209,0,0,0,49,192,0,0,17,254,0,0,17,51,192,0,17,51,224,0,17,19,32,14,16,2,32,9,16,0,32,24,144,0,32,8,112,0,32,4,48,0,32,2,16,0,32,1,0,0,32,0,128,0,32,0,64,0,32,0,32,0,32,0,16,0,96,0,12,0,192,0,7,131,128,0,0,254,0,0,0,0,0},
  {0,4,0,0,0,48,128,0,0,64,64,0,0,128,32,0,1,14,16,0,1,17,16,0,0,17,16,0,0,17,16,0,1,17,16,0,1,17,16,0,0,145,32,0,0,145,32,0,0,113,192,0,0,17,254,0,0,17,51,192,0,17,51,224,0,17,19,32,14,16,2,32,9,16,0,32,24,144,0,32,8,112,0,32,4,48,0,32,2,16,0,32,1,0,0,32,0,128,0,32,0,64,0,32,0,32,0,32,0,16,0,96,0,12,0,192,0,7,131,128,0,0,254,0,0,0,0,0},
  {0,4,0,0,0,48,128,0,0,64,64,0,0,128,32,0,1,12,16,0,1,26,16,0,0,19,16,0,0,17,16,0,1,17,16,0,1,17,16,0,0,145,32,0,0,145,32,0,0,113,192,0,0,17,254,0,0,17,51,192,0,17,51,96,0,17,19,32,14,16,2,32,9,16,0,32,24,144,0,32,8,112,0,32,4,48,0,32,2,16,0,32,1,0,0,32,0,128,0,32,0,64,0,32,0,32,0,32,0,16,0,96,0,12,0,192,0,7,255,128,0,0,124,0,0,0,0,0},
  {0,0,0,0,0,32,0,0,0,0,64,0,0,128,32,0,1,0,0,0,1,14,16,0,0,27,0,0,0,17,0,0,0,17,16,0,1,17,16,0,0,17,0,0,0,145,0,0,0,17,192,0,0,17,252,0,0,17,51,192,0,17,51,96,0,17,19,32,15,16,2,32,9,144,0,32,24,208,0,32,8,112,0,32,4,48,0,32,2,16,0,32,1,0,0,32,0,128,0,32,0,64,0,32,0,32,0,32,0,16,0,96,0,12,0,192,0,7,255,128,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,31,0,0,0,49,128,0,0,46,128,0,0,59,128,0,0,49,128,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,192,0,0,17,252,0,0,17,51,192,0,17,51,96,0,17,19,32,15,16,2,32,9,144,0,32,8,208,0,32,8,112,0,32,4,48,0,32,2,16,0,32,1,0,0,32,0,128,0,32,0,64,0,32,0,32,0,32,0,24,0,64,0,12,0,192,0,7,255,128,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,31,0,0,0,32,128,0,0,64,64,0,0,64,64,0,0,78,64,0,0,81,64,0,0,49,128,0,0,49,128,0,0,17,0,0,0,17,64,0,0,17,252,0,0,17,51,192,0,17,51,96,0,17,19,32,15,16,2,32,9,144,0,32,8,208,0,32,8,112,0,32,4,48,0,32,2,16,0,32,1,0,0,32,0,128,0,32,0,64,0,32,0,32,0,32,0,24,0,64,0,12,0,192,0,3,255,0,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,31,0,0,0,96,128,0,0,64,64,0,0,128,0,0,0,132,32,0,0,138,32,0,0,145,32,0,0,81,64,0,0,113,192,0,0,49,128,0,0,17,64,0,0,17,252,0,0,17,51,192,0,17,51,96,0,17,19,32,15,16,2,32,9,144,0,32,8,208,0,32,8,112,0,32,4,48,0,32,2,16,0,32,1,0,0,32,0,128,0,32,0,64,0,32,0,32,0,32,0,24,0,64,0,12,0,192,0,7,255,128,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,14,0,0,0,32,128,0,0,64,64,0,0,128,32,0,0,128,32,0,0,14,32,0,1,27,0,0,0,145,32,0,0,145,32,0,0,209,64,0,0,113,192,0,0,17,192,0,0,17,252,0,0,17,51,192,0,17,51,96,0,17,19,32,15,16,2,32,9,144,0,32,8,208,0,32,8,112,0,32,4,48,0,32,2,16,0,32,1,0,0,32,0,128,0,32,0,64,0,32,0,32,0,32,0,16,0,64,0,12,0,192,0,7,255,128,0,0,0,0,0,0,0,0},
  {0,0,0,0,0,27,0,0,0,64,192,0,0,128,32,0,0,128,32,0,1,12,0,0,1,26,16,0,1,17,16,0,1,17,16,0,1,17,0,0,0,145,32,0,0,81,64,0,0,49,192,0,0,17,254,0,0,17,51,192,0,17,51,96,0,17,19,32,15,16,2,32,9,144,0,32,24,208,0,32,8,112,0,32,4,48,0,32,2,16,0,32,1,0,0,32,0,128,0,32,0,64,0,32,0,32,0,32,0,16,0,96,0,12,0,192,0,7,255,128,0,0,124,0,0,0,0,0},
  {0,0,0,0,0,49,128,0,0,64,64,0,0,128,32,0,1,0,16,0,1,14,16,0,1,17,16,0,0,17,16,0,1,17,16,0,1,17,16,0,0,145,32,0,0,145,0,0,0,113,192,0,0,17,254,0,0,17,51,192,0,17,51,224,0,17,19,32,14,16,2,32,9,16,0,32,24,144,0,32,8,112,0,32,4,48,0,32,2,16,0,32,1,0,0,32,0,128,0,32,0,64,0,32,0,32,0,32,0,16,0,96,0,12,0,192,0,7,255,128,0,0,124,0,0,0,0,0},
  {0,4,0,0,0,48,128,0,0,64,64,0,0,128,32,0,1,12,16,0,1,26,16,0,0,17,16,0,0,17,16,0,1,17,16,0,1,17,16,0,0,145,32,0,0,145,32,0,0,113,192,0,0,17,254,0,0,17,51,192,0,17,51,224,0,17,19,32,14,16,2,32,9,16,0,32,24,144,0,32,8,112,0,32,4,48,0,32,2,16,0,32,1,0,0,32,0,128,0,32,0,64,0,32,0,32,0,32,0,16,0,96,0,12,0,192,0,7,255,128,0,0,126,0,0,0,0,0},
  {0,4,0,0,0,48,128,0,0,64,64,0,0,128,32,0,1,14,16,0,1,27,16,0,0,17,16,0,0,17,16,0,1,17,16,0,1,17,16,0,0,145,32,0,0,145,32,0,0,113,192,0,0,17,254,0,0,17,51,192,0,17,51,224,0,17,19,32,14,16,2,32,9,16,0,32,24,144,0,32,8,112,0,32,4,48,0,32,2,16,0,32,1,0,0,32,0,128,0,32,0,64,0,32,0,32,0,32,0,16,0,96,0,12,0,192,0,7,131,128,0,0,254,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,14,0,0,0,27,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,192,0,0,17,254,0,0,17,51,192,0,17,51,224,0,17,19,32,14,16,2,32,9,16,0,32,24,144,0,32,8,112,0,32,4,48,0,32,2,16,0,32,1,0,0,32,0,128,0,32,0,64,0,32,0,32,0,32,0,16,0,96,0,12,0,192,0,7,131,128,0,0,254,0,0,0,0,0},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,14,0,0,0,27,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,0,0,0,17,192,0,0,17,254,0,0,17,51,192,0,17,51,224,0,17,19,32,14,16,2,32,9,16,0,32,24,144,0,32,8,112,0,32,4,48,0,32,2,16,0,32,1,0,0,32,0,128,0,32,0,64,0,32,0,32,0,32,0,16,0,96,0,12,0,192,0,7,131,128,0,0,254,0,0,0,0,0}
};