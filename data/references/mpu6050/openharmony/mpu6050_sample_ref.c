#include "mpu6050_ref.h"
#include <stdio.h>
int mpu6050_openharmony_main(void) {
    struct mpu6050_device dev;
    DevHandle bus = I2cOpen(1);
    if (bus == NULL) { printf("[I2C] open bus FAILED\n"); return -1; }
    mpu6050_init(&dev, bus, MPU6050_ADDR_DEFAULT);
    if (mpu6050_probe(&dev) != 0) { printf("[MPU6050] probe FAILED\n"); return -1; }
    printf("[MPU6050] addr=0x%02X probe OK\n", MPU6050_ADDR_DEFAULT);
    for (int i = 0; i < 5; i++) {
        int16_t ax, ay, az, gx, gy, gz;
        if (mpu6050_read_accel(&dev, &ax, &ay, &az) == 0 &&
            mpu6050_read_gyro(&dev, &gx, &gy, &gz) == 0)
            printf("[MPU6050] sample=%d accel=(%d,%d,%d) gyro=(%d,%d,%d)\n", i+1,
                   ax, ay, az, gx, gy, gz);
    }
    I2cClose(bus);
    return 0;
}
