/*
 * DHT22 temperature/humidity sensor for NuttX (GPIO)
 */
#ifndef DHT22_REF_H
#define DHT22_REF_H

#include <nuttx/config.h>
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
    int data_fd;
};

int dht22_init(struct dht22_device *dev, const char *gpio_path);
void dht22_deinit(struct dht22_device *dev);
int dht22_read(struct dht22_device *dev, int16_t *temp_x10, uint16_t *humidity_x10);

#ifdef __cplusplus
}
#endif
#endif
