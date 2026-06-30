/*
 * MH-Z19B sample for Zephyr
 */
#include "mhz19b_ref.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mhz19b_sample, LOG_LEVEL_INF);

void main(void)
{
    struct mhz19b_device co2;
    const struct device *uart = DEVICE_DT_GET(DT_NODELABEL(uart1));

    if (mhz19b_init(&co2, uart) != 0)
    { LOG_ERR("Init failed"); return; }

    while (1) {
        uint16_t ppm;
        if (mhz19b_read_co2(&co2, &ppm) == 0)
            LOG_INF("CO2: %d ppm", ppm);
        k_msleep(2000);
    }
}
