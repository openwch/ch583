#ifndef __SENSOR_TASK_H
#define __SENSOR_TASK_H

#include "CH58x_common.h"

#define MPU9250_EVENT            0x0001
#define MAX30105_EVENT           0x0002


void Sensor_Task_Init(void);
extern uint8_t Sensor_TaskID;

#endif
