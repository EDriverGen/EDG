/*
 * ADXL345 sample for XiUOS
 */
#include "adxl345_ref.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
    struct adxl345_device acc;
    const char *path = (argc > 1) ? argv[1] : "/dev/spi0";
    if (adxl345_init(&acc, path, ADXL345_RANGE_2G) != 0)
    { printf("Init failed\n"); return -1; }
    for (int i = 0; i < 10; i++) {
        int32_t x, y, z;
        if (adxl345_read_accel_mg(&acc, &x, &y, &z) == 0)
            printf("X:%ld Y:%ld Z:%ld mg\n", (long)x, (long)y, (long)z);
        PrivTaskDelay(100);
    }
    adxl345_deinit(&acc);
    return 0;
}
