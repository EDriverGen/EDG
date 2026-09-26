#ifndef HCSR04_H
#define HCSR04_H

#include <stdint.h>
#include <stddef.h>

struct hcsr04_device {
    void *bus_handle;
    uint32_t trig_line;
    uint32_t echo_line;
};

int hcsr04_init(struct hcsr04_device *dev);
int hcsr04_read_distance(struct hcsr04_device *dev, int32_t *raw);

#endif /* HCSR04_H */