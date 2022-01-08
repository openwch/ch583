#include "I2C/myi2c.h"
#include "MPU9250.h"
#include "CH58x_common.h"

void scanI2Cdevice(void)
{
    uint8_t err, addr;
    int nDevices = 0;
    for (addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        err = Wire.endTransmission();
        if (err == 0) {
            PRINT("I2C device found at address ");
            if (addr < 16)
               PRINT("0");
            PRINT("%#x", addr);
            PRINT(" !\n");
            nDevices++;
        } else if (err == 4) {
            PRINT("Unknow error at address ");
            if (addr < 16)
                PRINT("0");
            PRINT("%#x\n", addr);
        }
    }
    if (nDevices == 0)
        PRINT("No I2C devices found\n");
    else
        PRINT("Done\n");
}

void test_mpu9250(void) {

    static bool is_init = false;

    if(is_init != true) {
        My_I2C_Init();

        scanI2Cdevice();

        setupMPU9250();

        is_init = true;
    }

//    while(1)
    {
        readMPU9250();

//        PRINT("Ax=%.2f,Ay=%.2f,Az=%.2f\n", (int)1000 * IMU.ax, (int)1000 *IMU.ay,(int)1000 * IMU.az);
//        PRINT("Gx=%.2f,Gy=%.2f,Gz=%.2f\n", IMU.gx, IMU.gy, IMU.gz);
//        PRINT("Mx=%.2f,My=%.2f,Mz=%.2f\n\n", IMU.mz, IMU.my, IMU.mz);


        PRINT("pitch=%.2f,yaw=%.2f,roll=%.2f\n", IMU.pitch, IMU.yaw, IMU.roll);
        DelayMs(10);
    }

}
