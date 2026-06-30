#include "mpu6050_ref.h"
#include <stdio.h>
extern I2C_HandleTypeDef hi2c1;

int mpu6050_tobudos_main(void) {
    struct mpu6050_device dev;
    mpu6050_init(&dev, &hi2c1, MPU6050_ADDR_DEFAULT);
    if (mpu6050_probe(&dev) != 0) { printf("[MPU6050] probe FAILED\n"); return -1; }
    printf("[MPU6050] addr=0x%02X probe OK\n", MPU6050_ADDR_DEFAULT);
    for (int i = 0; i < 5; i++) {
        int16_t ax, ay, az, gx, gy, gz;
        if (mpu6050_read_accel(&dev, &ax, &ay, &az) == 0 &&
            mpu6050_read_gyro(&dev, &gx, &gy, &gz) == 0)
            printf("[MPU6050] sample=%d accel=(%d,%d,%d) gyro=(%d,%d,%d)\n", i+1,
                   ax, ay, az, gx, gy, gz);
    }
    return 0;
}
