#include "max31855_ref.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(max31855_sample, LOG_LEVEL_INF);

void main(void) {
    struct max31855_device tc;
    const struct device *spi = DEVICE_DT_GET(DT_NODELABEL(spi1));
    if (max31855_init(&tc, spi, NULL) != 0) { LOG_ERR("Init failed"); return; }
    for (int i = 0; i < 5; i++) {
        int32_t t; if (max31855_read_thermocouple(&tc, &t) == 0)
            LOG_INF("TC: %d.%03d C", t/1000, (t>=0?t:-t)%1000);
        k_msleep(1000);
    }
}
