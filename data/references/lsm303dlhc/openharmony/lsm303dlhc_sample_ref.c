#include "lsm303dlhc_ref.h"
#include <stdio.h>

int lsm303dlhc_openharmony_main(void) {
    struct lsm303dlhc_device dev;
    DevHandle bus = I2cOpen(1);
    if (bus == NULL) { printf("[I2C] open bus FAILED\n"); return -1; }
    lsm303dlhc_init(&dev, bus, LSM303DLHC_ADDR_ACCEL);
    if (lsm303dlhc_probe(&dev) != 0) { printf("[LSM303DLHC] probe FAILED\n"); return -1; }
    printf("[LSM303DLHC] accel=0x%02X mag=0x%02X probe OK\n", LSM303DLHC_ADDR_ACCEL, LSM303DLHC_ADDR_MAG);
    for (int i = 0; i < 5; i++) {
        int16_t ax, ay, az, mx, my, mz;
        lsm303dlhc_enable_accel(&dev);
        lsm303dlhc_enable_mag(&dev);
        if (lsm303dlhc_read_accel(&dev, &ax, &ay, &az) == 0 &&
            lsm303dlhc_read_mag(&dev, &mx, &my, &mz) == 0)
            printf("[LSM303DLHC] sample=%d accel=(%d,%d,%d) mag=(%d,%d,%d)\n", i+1,
                   ax, ay, az, mx, my, mz);
    }
    I2cClose(bus);
    return 0;
}
