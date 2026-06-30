/*
 * DHT22 temperature/humidity sensor for TencentOS Tiny (GPIO)
 */
#ifndef DHT22_REF_H
#define DHT22_REF_H

#include "tos_k.h"
#include "stm32f1xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DHT22_START_LOW_US    1000
#define DHT22_TIMEOUT_US      200
#define DHT22_BIT_THRESHOLD   40   /* >40us high = bit 1 */
#define DHT22_MIN_INTERVAL_MS 2000

struct dht22_device
{
    GPIO_TypeDef *data_port;
    uint16_t data_pin;
};

int dht22_init(struct dht22_device *dev, GPIO_TypeDef *port, uint16_t pin);
int dht22_read(struct dht22_device *dev, int16_t *temp_x10, uint16_t *humidity_x10);
extern void dht22_delay_us(uint32_t us);

#ifdef __cplusplus
}
#endif
#endif
