/*
 * ADXL345 sample for RIOT
 */
#include "adxl345_ref.h"
#include <stdio.h>
#include "xtimer.h"

int main(void)
{
    struct adxl345_device acc;
    adxl345_init(&acc, SPI_DEV(0), GPIO_PIN(0, 4), ADXL345_RANGE_2G);

    while (1) {
        int32_t x, y, z;
        if (adxl345_read_accel_mg(&acc, &x, &y, &z) == 0)
            printf("X:%ld Y:%ld Z:%ld mg\n", (long)x, (long)y, (long)z);
        xtimer_msleep(100);
    }
    return 0;
}
