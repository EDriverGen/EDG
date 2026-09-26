#ifndef DHT22_H
#define DHT22_H

#include <stdint.h>
#include <stddef.h>

struct dht22_device {
    void *bus_handle;
    uint16_t trig_pin;
    uint16_t echo_pin;
};

int dht22_init(struct dht22_device *dev, void *bus_name);
int dht22_read_sensor(struct dht22_device *dev, int32_t *humidity_val, int32_t *temp_val);

#endif /* DHT22_H */