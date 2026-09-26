/*
 * DHT22 temperature/humidity sensor for ThreadX (HAL-agnostic)
 */
#ifndef DHT22_REF_H
#define DHT22_REF_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DHT22_START_LOW_US    1000
#define DHT22_TIMEOUT_US      200
#define DHT22_BIT_THRESHOLD   40   /* >40us high = bit 1 */
#define DHT22_MIN_INTERVAL_MS 2000

struct dht22_gpio_ops
{
    void (*set_output)(void *ctx);
    void (*set_input)(void *ctx);
    void (*write)(void *ctx, int val);
    int  (*read)(void *ctx);
    void (*delay_us)(uint32_t us);
    void (*delay_ms)(uint32_t ms);
};

struct dht22_device
{
    const struct dht22_gpio_ops *ops;
    void *ctx;
};

int dht22_init(struct dht22_device *dev, const struct dht22_gpio_ops *ops, void *ctx);
int dht22_read(struct dht22_device *dev, int16_t *temp_x10, uint16_t *humidity_x10);

#ifdef __cplusplus
}
#endif
#endif
