#ifndef count_steps_h
#define count_steps_h
#include "stdint.h"

#define SAMPLING_RATE           20                       //20 hz sampling rate
#define NUM_TUPLES              80                       //80 sets of accelerometer readings (so in other words, 80*3 = 240 samples)
#define WINDOW_LENGTH           NUM_TUPLES/SAMPLING_RATE //window length in seconds

uint8_t count_steps(int8_t *data);

#endif /* count_steps_h */