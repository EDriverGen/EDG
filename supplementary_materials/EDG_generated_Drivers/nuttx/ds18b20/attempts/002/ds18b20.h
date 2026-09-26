#ifndef DS18B20_H
#define DS18B20_H

#include <stdint.h>
#include <stdbool.h>

struct ds18b20_dev {
    int trig_fd;
    int echo_fd;
};

int ds18b20_init(struct ds18b20_dev *dev, const char *trig_path, const char *echo_path);
int ds18b20_read_temperature(struct ds18b20_dev *dev, int32_t *raw);

#endif /* DS18B20_H */