/*
 * DHT22 temperature/humidity sensor for RIOT (GPIO)
 */
#ifndef DHT22_REF_H
#define DHT22_REF_H

#include "periph/gpio.h"
#include "xtimer.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DHT22_START_LOW_US    1000
#define DHT22_TIMEOUT_US      200
#define DHT22_BIT_THRESHOLD   40   /* >40us high = bit 1 */
#define DHT22_MIN_INTERVAL_MS 2000

struct dht22_device
{
    gpio_t data_pin;
};

int dht22_init(struct dht22_device *dev, gpio_t pin);
int dht22_read(struct dht22_device *dev, int16_t *temp_x10, uint16_t *humidity_x10);

#ifdef __cplusplus
}
#endif
#endif
