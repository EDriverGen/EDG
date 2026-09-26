#ifndef DS18B20_H
#define DS18B20_H

#include <stdint.h>

#include <hal/hal_gpio.h>
struct ds18b20_dev {
    int data_pin;
};

int ds18b20_init(struct ds18b20_dev *dev, int pin);
int ds18b20_read_temperature(struct ds18b20_dev *dev, int32_t *raw);

#endif
