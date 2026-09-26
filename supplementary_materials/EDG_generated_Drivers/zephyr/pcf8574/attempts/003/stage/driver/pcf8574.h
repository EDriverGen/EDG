#ifndef PCF8574_H
#define PCF8574_H

#include <stdint.h>
#include <zephyr/device.h>

#include <zephyr/drivers/i2c.h>
struct device;

int pcf8574_init(const struct device *dev, const struct device *bus);
int pcf8574_read_port(const struct device *dev, uint8_t *p0, uint8_t *p1, uint8_t *p2, uint8_t *p3, uint8_t *p4, uint8_t *p5, uint8_t *p6, uint8_t *p7);

#endif /* PCF8574_H */
