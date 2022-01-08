#ifndef _QUATERNIONFILTERS_H_
#define _QUATERNIONFILTERS_H_

#define PI      3.1415926535897932384626433832795
#define DEG_TO_RAD          0.017453292519943295769236907684886
#define RAD_TO_DEG          57.295779513082320876798154814105

void MadgwickQuaternionUpdate(float ax, float ay, float az, float gx, float gy,
                              float gz, float mx, float my, float mz,
                              float deltat);
void MahonyQuaternionUpdate(float ax, float ay, float az, float gx, float gy,
                            float gz, float mx, float my, float mz,
                            float deltat);
const float * getQ();

#endif // _QUATERNIONFILTERS_H_
