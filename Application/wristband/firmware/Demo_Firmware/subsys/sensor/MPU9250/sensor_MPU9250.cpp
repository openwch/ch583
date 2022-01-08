#include "MPU9250.h"
#include "quaternionFilters.h"
#include <math.h>

extern "C"{
    #include "count_steps.h"
    #include "CH58x_common.h"
}


#define AHRS true         // Set to false for basic data read
#define SerialDebugMPU9250   false  // Set to true to get Serial output for debugging
#define SerialDebugCalibrate true  // Set to true to get Serial output for debugging

MPU9250 IMU;

bool setupMPU9250() {

  Wire.begin();

//  scanI2Cdevice();

  uint8_t c = IMU.readByte(MPU9250_ADDRESS, WHO_AM_I_MPU9250);
  PRINT("MPU9250 "); PRINT("I AM "); PRINT("%#x\n", c);
  PRINT(" I should be "); PRINT("%#x\n", 0x71);
  if (c == 0x71) {
      PRINT("MPU9250 is online...\n");
    // Start by performing self test and reporting values
    IMU.MPU9250SelfTest(IMU.SelfTest);
    if (SerialDebugCalibrate) {
      PRINT("x-axis self test: acceleration trim within : ");
      PRINT("%.2f ",(uint8_t)IMU.SelfTest[0]); PRINT("%% of factory value\n");
      PRINT("y-axis self test: acceleration trim within : ");
      PRINT("%.2f ",IMU.SelfTest[1]); PRINT("%% of factory value\n");
      PRINT("z-axis self test: acceleration trim within : ");
      PRINT("%.2f ",IMU.SelfTest[2]); PRINT("%% of factory value\n");
      PRINT("x-axis self test: gyration trim within : ");
      PRINT("%.2f ",IMU.SelfTest[3]); PRINT("%% of factory value\n");
      PRINT("y-axis self test: gyration trim within : ");
      PRINT("%.2f ",IMU.SelfTest[4]); PRINT("%% of factory value\n");
      PRINT("z-axis self test: gyration trim within : ");
      PRINT("%.2f ",IMU.SelfTest[5]); PRINT("%% of factory value\n");
    }
    PRINT("MPU9250 acceleration and gyration self test done!\n");

    // Calibrate gyro and accelerometers, load biases in bias registers
    IMU.calibrateMPU9250(IMU.gyroBias, IMU.accelBias);

    IMU.initMPU9250();
    // Initialize device for active mode read of acclerometer, gyroscope, and
    // temperature
    PRINT("MPU9250 initialized for active data mode....\n");

    // Read the WHO_AM_I register of the magnetometer, this is a good test of
    // communication
    uint8_t d = IMU.readByte(AK8963_ADDRESS, WHO_AM_I_AK8963);
    PRINT("AK8963 "); PRINT("I AM "); PRINT("%#x\n", d);
    PRINT(" I should be "); PRINT("%#x\n", 0x48);


    // Get magnetometer calibration from AK8963 ROM
    IMU.initAK8963(IMU.magCalibration);
    // Initialize device for active mode read of magnetometer
    PRINT("AK8963 initialized for active data mode....\n");
    if (SerialDebugCalibrate) {
      PRINT("Calibration values: \n");
      PRINT("X-Axis sensitivity adjustment value ");
      PRINT("%.2f\n", IMU.magCalibration[0]);
      PRINT("Y-Axis sensitivity adjustment value ");
      PRINT("%.2f\n", IMU.magCalibration[1]);
      PRINT("Z-Axis sensitivity adjustment value ");
      PRINT("%.2f\n", IMU.magCalibration[2]);
    }
    PRINT("AK8963 magCalibration done!\n");
  }
  else {
      return false;
  }
  return true;
}

void sleepMPU9250()
{
    IMU.setSleepEnabled(ENABLE);
}

void WakeMPU9250()
{
    IMU.setSleepEnabled(DISABLE);
}

int32_t readMPU9250_step( int8_t *data ) {
    if (IMU.readByte(MPU9250_ADDRESS, INT_STATUS) & 0x01)
    {
      IMU.readAccelData(IMU.accelCount);  // Read the x/y/z adc values
      IMU.getAres();

      // Now we'll calculate the accleration value into actual g's
      // This depends on scale being set
      IMU.ax = (float)IMU.accelCount[0] * IMU.aRes; // - accelBias[0];
      IMU.ay = (float)IMU.accelCount[1] * IMU.aRes; // - accelBias[1];
      IMU.az = (float)IMU.accelCount[2] * IMU.aRes; // - accelBias[2];

      IMU.readGyroData(IMU.gyroCount);  // Read the x/y/z adc values
      IMU.getGres();

      // Calculate the gyro value into actual degrees per second
      // This depends on scale being set
      IMU.gx = (float)IMU.gyroCount[0] * IMU.gRes;
      IMU.gy = (float)IMU.gyroCount[1] * IMU.gRes;
      IMU.gz = (float)IMU.gyroCount[2] * IMU.gRes;

      IMU.readMagData(IMU.magCount);  // Read the x/y/z adc values
      IMU.getMres();
      // User environmental x-axis correction in milliGauss, should be
      // automatically calculated
      IMU.magbias[0] = +470.;
      // User environmental x-axis correction in milliGauss TODO axis??
      IMU.magbias[1] = +120.;
      // User environmental x-axis correction in milliGauss
      IMU.magbias[2] = +125.;

      // Calculate the magnetometer values in milliGauss
      // Include factory calibration per data sheet and user environmental
      // corrections
      // Get actual magnetometer value, this depends on scale being set
      IMU.mx = (float)IMU.magCount[0] * IMU.mRes * IMU.magCalibration[0] -
               IMU.magbias[0];
      IMU.my = (float)IMU.magCount[1] * IMU.mRes * IMU.magCalibration[1] -
               IMU.magbias[1];
      IMU.mz = (float)IMU.magCount[2] * IMU.mRes * IMU.magCalibration[2] -
               IMU.magbias[2];
    } // if (readByte(MPU9250_ADDRESS, INT_STATUS) & 0x01)

    static uint16_t i    = 0;
    float    temp = 0;

//    int *acc = (int *)malloc(sizeof(int) * 240);
    //scaling factor to convert the decimal data to int8 integers. calculated in matlab by taking the absolute value of all the data
    //and then calculating the max of that data. then divide that by 127 to get the scaling factor
    float scale_factor = 55.3293;

    temp     = roundf(IMU.ax * scale_factor);
    data[i++] = (int8_t)temp;

    temp     = roundf(IMU.ay * scale_factor);
    data[i++] = (int8_t)temp;

    temp     = roundf(IMU.az * scale_factor);
    data[i++] = (int8_t)temp;

    if(i >= 240) {
        int32_t  num_steps = count_steps(data);
        i = 0;
        return num_steps;
    }

    return -1;
}

void readMPU9250() {
  // If intPin goes high, all data registers have new data
  // On interrupt, check if data ready interrupt
  if (IMU.readByte(MPU9250_ADDRESS, INT_STATUS) & 0x01)
  {
    IMU.readAccelData(IMU.accelCount);  // Read the x/y/z adc values
    IMU.getAres();

    // Now we'll calculate the accleration value into actual g's
    // This depends on scale being set
    IMU.ax = (float)IMU.accelCount[0] * IMU.aRes; // - accelBias[0];
    IMU.ay = (float)IMU.accelCount[1] * IMU.aRes; // - accelBias[1];
    IMU.az = (float)IMU.accelCount[2] * IMU.aRes; // - accelBias[2];

    IMU.readGyroData(IMU.gyroCount);  // Read the x/y/z adc values
    IMU.getGres();

    // Calculate the gyro value into actual degrees per second
    // This depends on scale being set
    IMU.gx = (float)IMU.gyroCount[0] * IMU.gRes;
    IMU.gy = (float)IMU.gyroCount[1] * IMU.gRes;
    IMU.gz = (float)IMU.gyroCount[2] * IMU.gRes;

    IMU.readMagData(IMU.magCount);  // Read the x/y/z adc values
    IMU.getMres();
    // User environmental x-axis correction in milliGauss, should be
    // automatically calculated
    IMU.magbias[0] = +470.;
    // User environmental x-axis correction in milliGauss TODO axis??
    IMU.magbias[1] = +120.;
    // User environmental x-axis correction in milliGauss
    IMU.magbias[2] = +125.;

    // Calculate the magnetometer values in milliGauss
    // Include factory calibration per data sheet and user environmental
    // corrections
    // Get actual magnetometer value, this depends on scale being set
    IMU.mx = (float)IMU.magCount[0] * IMU.mRes * IMU.magCalibration[0] -
             IMU.magbias[0];
    IMU.my = (float)IMU.magCount[1] * IMU.mRes * IMU.magCalibration[1] -
             IMU.magbias[1];
    IMU.mz = (float)IMU.magCount[2] * IMU.mRes * IMU.magCalibration[2] -
             IMU.magbias[2];
  } // if (readByte(MPU9250_ADDRESS, INT_STATUS) & 0x01)

  static uint16_t i    = 0;
  float    temp = 0;
  static int8_t data[245];
//    int *acc = (int *)malloc(sizeof(int) * 240);
  //scaling factor to convert the decimal data to int8 integers. calculated in matlab by taking the absolute value of all the data
  //and then calculating the max of that data. then divide that by 127 to get the scaling factor
  float scale_factor = 55.3293;

  temp     = roundf(IMU.ax * scale_factor);
  data[i++] = (int8_t)temp;

  temp     = roundf(IMU.ay * scale_factor);
  data[i++] = (int8_t)temp;

  temp     = roundf(IMU.az * scale_factor);
  data[i++] = (int8_t)temp;

  if(i >= 240) {
      i = 0;
      IMU.num_steps += count_steps(data);
  }

  // Must be called before updating quaternions!
  IMU.updateTime();


  // Sensors x (y)-axis of the accelerometer is aligned with the y (x)-axis of
  // the magnetometer; the magnetometer z-axis (+ down) is opposite to z-axis
  // (+ up) of accelerometer and gyro! We have to make some allowance for this
  // orientationmismatch in feeding the output to the quaternion filter. For the
  // MPU-9250, we have chosen a magnetic rotation that keeps the sensor forward
  // along the x-axis just like in the LSM9DS0 sensor. This rotation can be
  // modified to allow any convenient orientation convention. This is ok by
  // aircraft orientation standards! Pass gyro rate as rad/s
  //  MadgwickQuaternionUpdate(ax, ay, az, gx*PI/180.0f, gy*PI/180.0f, gz*PI/180.0f,  my,  mx, mz);
  MahonyQuaternionUpdate(IMU.ax, IMU.ay, IMU.az, IMU.gx * DEG_TO_RAD,
                         IMU.gy * DEG_TO_RAD, IMU.gz * DEG_TO_RAD, IMU.my,
                         IMU.mx, IMU.mz, IMU.deltat);
  // Serial print and/or display at 0.5 s rate independent of data rates
  IMU.delt_t = millis() - IMU.count;

  if (IMU.delt_t > 20) {
    if (SerialDebugMPU9250) {
      PRINT("ax=");
      PRINT("%.2f", (int)1000 * IMU.ax);
      PRINT(",ay=");
      PRINT("%.2f", (int)1000 * IMU.ay);
      PRINT(",az=");
      PRINT("%.2f", (int)1000 * IMU.az);
      PRINT("\n");
//      PRINT(" mg\n");

      PRINT("gx=");
      PRINT("%.2f", IMU.gx);
      PRINT(",gy=");
      PRINT("%.2f", IMU.gy);
      PRINT(",gz=");
      PRINT("%.2f", IMU.gz);
      PRINT("\n");
//      PRINT(" deg/s\n");

      PRINT("mx=");
      PRINT("%.2f", (int)IMU.mx );
      PRINT(",my=");
      PRINT("%.2f", (int)IMU.my );
      PRINT(",mz=");
      PRINT("%.2f", (int)IMU.mz );
      PRINT("\n");
//      PRINT(" mG\n");

      //Serial.print("q0 = ");
      PRINT("q0 = %.2f\n", *getQ());
      // Serial.print(" qx = ");
      PRINT("qx = %.2f\n",*(getQ() + 1));
      // Serial.print(" qy = ");
      PRINT("qy = %.2f\n",*(getQ() + 2));
      // Serial.print(" qz = ");
      PRINT("qz = %.2f\n",*(getQ() + 3));
    }

    IMU.yaw   = atan2(2.0f * (*(getQ() + 1) * *(getQ() + 2) + *getQ() *
                              *(getQ() + 3)), *getQ() * *getQ() + * (getQ() + 1) * *(getQ() + 1)
                      - * (getQ() + 2) * *(getQ() + 2) - * (getQ() + 3) * *(getQ() + 3));
    IMU.pitch = -asin(2.0f * (*(getQ() + 1) * *(getQ() + 3) - *getQ() *
                              *(getQ() + 2)));
    IMU.roll  = atan2(2.0f * (*getQ() * *(getQ() + 1) + * (getQ() + 2) *
                              *(getQ() + 3)), *getQ() * *getQ() - * (getQ() + 1) * *(getQ() + 1)
                      - * (getQ() + 2) * *(getQ() + 2) + * (getQ() + 3) * *(getQ() + 3));
    IMU.pitch *= RAD_TO_DEG;
    IMU.yaw   *= RAD_TO_DEG;
    // Declination of SparkFun Electronics (40бу05'26.6"N 105бу11'05.9"W)is
    //  8бу 30' E  б└ 0бу 21' (or 8.5бу) on 2016-07-19
    // - http://www.ngdc.noaa.gov/geomag-web/#declination
    IMU.yaw   -= 8.5;
    IMU.roll  *= RAD_TO_DEG;

    if (SerialDebugMPU9250) {
      PRINT("Yaw, Pitch, Roll: ");
      PRINT("%.2f", IMU.yaw);
      PRINT(", ");
      PRINT("%.2f", IMU.pitch);
      PRINT(", ");
      PRINT("%.2f\n", IMU.roll);
    }

    IMU.count = millis();
    IMU.sumCount = 0;
    IMU.sum = 0;

  } // if (IMU.delt_t > 20)
}

// void get_cursor_position(uint8_t *x, uint8_t *y) {
//   int cursor_x = SCREEN_WIDTH / 2 + (int)(SCREEN_WIDTH / 2 * (IMU.roll / MAX_CURSOR_ACC));
//   int cursor_y = SCREEN_HEIGHT / 2 - (int)(SCREEN_HEIGHT / 2 * (IMU.pitch / MAX_CURSOR_ACC));

//   if (cursor_x < 0 ) {
//     cursor_x = 0;
//   }
//   if (cursor_x >= SCREEN_WIDTH) {
//     cursor_x = SCREEN_WIDTH - 1;
//   }
//   if (cursor_y < 0) {
//     cursor_y = 0;
//   }
//   if (cursor_y >= SCREEN_HEIGHT - 1) {
//     cursor_y = SCREEN_HEIGHT - 2;
//   }

//   *x = cursor_x;
//   *y = cursor_y;
// }

