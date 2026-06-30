/*
 * ADXL345 sample for ThreadX
 */
#include "adxl345_ref.h"
#include <stdio.h>

extern const struct adxl345_spi_ops platform_spi_ops;
extern void *platform_spi_ctx;

void adxl345_sample_entry(ULONG param)
{
    (void)param;
    struct adxl345_device acc;
    if (adxl345_init(&acc, &platform_spi_ops, platform_spi_ctx, ADXL345_RANGE_2G) != 0)
    { printf("Init failed\r\n"); return; }
    for (int i = 0; i < 10; i++) {
        int32_t x, y, z;
        if (adxl345_read_accel_mg(&acc, &x, &y, &z) == 0)
            printf("X:%ld Y:%ld Z:%ld mg\r\n", (long)x, (long)y, (long)z);
    }
}
