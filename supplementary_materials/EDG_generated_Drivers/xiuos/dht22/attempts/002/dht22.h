#ifndef DHT22_H
#define DHT22_H

#include <stdint.h>

struct dht22_device {
    int trig_fd;
    int echo_fd;
};

int dht22_init(struct dht22_device *dev);
int dht22_read_sensor(struct dht22_device *dev, int32_t *humidity, int32_t *temperature);

#endif