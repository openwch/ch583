#ifndef _SENSOR_MAX30102_H_
#define _SENSOR_MAX30102_H_


int read_HeartRate(float *beatsPerMinute, int *beatAvg);
int read_SPO2(void);
bool setupMax30102(void);
void shutMax30102(void);
void WakeMax30102(void);

#endif // _SENSOR_MAX30102_H_
