#ifndef MHZ19B_H
#define MHZ19B_H

#include <stdint.h>
#include <stddef.h>
#include <zephyr/device.h>

#include <zephyr/drivers/uart.h>
struct device;

int mhz19b_init(const struct device *dev);
int mhz19b_read_co2(const struct device *dev, int32_t *raw);

#endif /* MHZ19B_H */
