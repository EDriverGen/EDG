/*
 * ADXL345 sample for NuttX
 */
#include "adxl345_ref.h"
#include <stdio.h>

int main(int argc, FAR char *argv[])
{
    struct adxl345_device acc;
    FAR struct spi_dev_s *spi = up_spiinitialize(0);
    if (!spi) { printf("SPI init failed\n"); return -1; }
    if (adxl345_init(&acc, spi, 0, ADXL345_RANGE_2G) != 0)
    { printf("ADXL345 init failed\n"); return -1; }
    for (int i = 0; i < 10; i++) {
        int32_t x, y, z;
        if (adxl345_read_accel_mg(&acc, &x, &y, &z) == 0)
            printf("X:%ld Y:%ld Z:%ld mg\n", (long)x, (long)y, (long)z);
        usleep(100000);
    }
    return 0;
}
