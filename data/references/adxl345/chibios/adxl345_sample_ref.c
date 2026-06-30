/*
 * ADXL345 sample for ChibiOS
 */
#include "adxl345_ref.h"
#include "chprintf.h"

void adxl345_sample_thread(void *arg)
{
    (void)arg;
    struct adxl345_device acc;
    static const SPIConfig spi_cfg = { NULL, GPIOA, 4,
        SPI_CR1_BR_1 | SPI_CR1_CPOL | SPI_CR1_CPHA };

    adxl345_init(&acc, &SPID1, &spi_cfg, ADXL345_RANGE_2G);
    while (true) {
        int32_t x, y, z;
        if (adxl345_read_accel_mg(&acc, &x, &y, &z) == 0)
            chprintf((BaseSequentialStream *)&SD1, "X:%d Y:%d Z:%d mg\r\n", x, y, z);
        chThdSleepMilliseconds(100);
    }
}
