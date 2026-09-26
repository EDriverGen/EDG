#ifndef DHT22_H
#define DHT22_H

#include <stdint.h>
#include "stm32f1xx_hal_gpio.h"

struct dht22_device {
    GPIO_TypeDef *port;
    uint16_t pin;
};

int dht22_init(struct dht22_device *dev, void *bus_name);
int dht22_read_sensor(struct dht22_device *dev, int32_t *humidity_val, int32_t *temp_val);

#endif /* DHT22_H */