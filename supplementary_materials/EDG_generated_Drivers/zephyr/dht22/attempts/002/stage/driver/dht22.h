#ifndef DHT22_H
#define DHT22_H

#include <stdint.h>
#include <zephyr/drivers/gpio.h>

struct device;

struct dht22_data {
    const struct device *gpio_dev;
    struct gpio_dt_spec gpio;
};

int dht22_init(struct dht22_data *dev, const struct device *gpio_dev);
int dht22_read(struct dht22_data *dev, int32_t *humidity_val, int32_t *temp_val);

#endif /* DHT22_H */