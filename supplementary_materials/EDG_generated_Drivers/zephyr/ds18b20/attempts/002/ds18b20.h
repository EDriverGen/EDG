#ifndef DS18B20_H
#define DS18B20_H

#include <stdint.h>
#include <zephyr/drivers/gpio.h>

struct device;

struct ds18b20_dev {
    const struct device *port;
    gpio_pin_t pin;
};

int ds18b20_init(struct ds18b20_dev *dev, const struct device *port, gpio_pin_t pin);
int ds18b20_read_temperature(struct ds18b20_dev *dev, int32_t *raw);

#endif /* DS18B20_H */