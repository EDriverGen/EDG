#ifndef DHT22_RTEMS_REF_H
#define DHT22_RTEMS_REF_H

#include <stdint.h>
#include <rtems.h>
#include <rtems/gpio.h>

#define DHT22_TIMEOUT_US    200U
#define DHT22_BIT_THRESHOLD 40U

struct dht22_device {
    rtems_gpio_pin data_pin;
};

int dht22_init(struct dht22_device *dev, rtems_gpio_pin data_pin);
int dht22_read(struct dht22_device *dev, int16_t *temp_x10, uint16_t *humidity_x10);
int dht22_decode_frame(const unsigned char raw_frame[5], short *temp_x10,
                       unsigned short *humidity_x10);

#endif
