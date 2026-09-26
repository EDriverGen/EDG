#ifndef DHT22_H
#define DHT22_H

#include <stdint.h>

struct dht22_dev {
    int data_pin;
};

int dht22_init(struct dht22_dev *dev, int data_pin);
int dht22_read_sensor(struct dht22_dev *dev, int32_t *humidity, int32_t *temperature);

#endif