#include "mpu6050_ref.h"
#include <stdio.h>
extern void *g_mpu6050_i2c_context;
extern const struct mpu6050_i2c_ops g_mpu6050_i2c_ops;
int mpu6050_threadx_main(void) {
    struct mpu6050_device dev;
    mpu6050_init(&dev, g_mpu6050_i2c_context, &g_mpu6050_i2c_ops, MPU6050_ADDR_DEFAULT);
    if (mpu6050_probe(&dev) != 0) { printf("[MPU6050] probe FAILED\n"); return -1; }
    printf("[MPU6050] addr=0x%02X probe OK\n", MPU6050_ADDR_DEFAULT);
    for (int i = 0; i < 3; i++) {
        int16_t ax,ay,az,gx,gy,gz;
        if (mpu6050_read_accel(&dev, &ax, &ay, &az) == 0 &&
            mpu6050_read_gyro(&dev, &gx, &gy, &gz) == 0)
            printf("[MPU6050] accel=(%d,%d,%d) gyro=(%d,%d,%d)\n",
                   ax, ay, az, gx, gy, gz);
    }
    return 0;
}
