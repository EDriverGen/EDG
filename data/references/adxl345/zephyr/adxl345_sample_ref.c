/*
 * ADXL345 sample for Zephyr
 */
#include "adxl345_ref.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(adxl345_sample, LOG_LEVEL_INF);

void main(void)
{
    struct adxl345_device acc;
    const struct device *spi = DEVICE_DT_GET(DT_NODELABEL(spi1));

    if (adxl345_init(&acc, spi, NULL, ADXL345_RANGE_2G) != 0)
    { LOG_ERR("Init failed"); return; }

    while (1) {
        int32_t x, y, z;
        if (adxl345_read_accel_mg(&acc, &x, &y, &z) == 0)
            LOG_INF("X:%d Y:%d Z:%d mg", x, y, z);
        k_msleep(100);
    }
}
