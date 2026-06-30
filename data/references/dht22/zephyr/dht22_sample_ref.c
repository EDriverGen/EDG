/*
 * DHT22 sample for Zephyr
 */
#include "dht22_ref.h"
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(dht22_sample, LOG_LEVEL_INF);

static const struct gpio_dt_spec dht_pin = GPIO_DT_SPEC_GET(DT_NODELABEL(dht22), gpios);

void main(void)
{
    struct dht22_device sensor;
    if (dht22_init(&sensor, &dht_pin) != 0)
    { LOG_ERR("Init failed"); return; }

    while (1) {
        int16_t temp; uint16_t hum;
        if (dht22_read(&sensor, &temp, &hum) == 0)
            LOG_INF("T:%d.%d C  H:%d.%d %%", temp/10, (temp>=0?temp:-temp)%10, hum/10, hum%10);
        k_msleep(DHT22_MIN_INTERVAL_MS);
    }
}
