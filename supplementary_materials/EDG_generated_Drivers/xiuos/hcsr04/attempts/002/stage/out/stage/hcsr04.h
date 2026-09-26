#ifndef HCSR04_H
#define HCSR04_H

#include <stdint.h>

struct hcsr04_device {
    int trig_fd;
    int echo_fd;
};

int hcsr04_init(struct hcsr04_device *dev, const char *trig_path, const char *echo_path);
int hcsr04_read_distance(struct hcsr04_device *dev, int32_t *raw);

#endif /* HCSR04_H */