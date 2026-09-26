#ifndef DHT22_H
#define DHT22_H

#include <stdint.h>

struct dht22_dev {
    int trig_fd;
    int echo_fd;
};

int dht22_init(struct dht22_dev *dev, const char *trig_path, const char *echo_path);
int dht22_read_sensor(struct dht22_dev *dev, int32_t *humidity_val, int32_t *temp_val);

#endif /* DHT22_H */