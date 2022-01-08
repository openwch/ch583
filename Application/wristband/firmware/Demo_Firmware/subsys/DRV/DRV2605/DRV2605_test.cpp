#include "Adafruit_DRV2605.h"
#include "CH58x_common.h"

Adafruit_DRV2605 drv;

void DRV2605_test( void ) {

    LOG_INFO("DRV test");
    drv.begin();

    // I2C trigger by sending 'go' command
    drv.setMode(DRV2605_MODE_INTTRIG); // default, internal trigger when sending GO command

    drv.selectLibrary(1);
    drv.setWaveform(0, 84);  // ramp up medium 1, see datasheet part 11.2
    drv.setWaveform(1, 1);  // strong click 100%, see datasheet part 11.2
    drv.setWaveform(2, 0);  // end of waveforms

    while(1) {
        drv.go();
        DelayMs(1000);
    }
}
