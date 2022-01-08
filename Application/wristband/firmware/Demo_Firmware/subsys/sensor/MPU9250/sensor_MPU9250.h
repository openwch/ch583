#ifndef _SENSOR_MPU9250_H_
#define _SENSOR_MPU9250_H_

#include <stdint.h>

bool setupMPU9250(void);
void sleepMPU9250(void);
void WakeMPU9250(void);

void readMPU9250(void);
int32_t readMPU9250_step( int8_t *data );

#endif // _SENSOR_MPU9250_H_
