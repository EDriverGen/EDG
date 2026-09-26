#ifndef HCSR04_H
#define HCSR04_H

#include <stdint.h>

struct hcsr04_dev {
    int trig_fd;
    int echo_fd;
};

int hcsr04_init(struct hcsr04_dev *dev, const char *trig_path, const char *echo_path);
int hcsr04_read_distance(struct hcsr04_dev *dev, int32_t *raw);

#endif /* HCSR04_H */