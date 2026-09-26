/*
 * DHT22 (AM2302) temperature/humidity sensor for RT-Thread (GPIO)
 */
#ifndef DRIVERS_INCLUDE_DHT22_H_
#define DRIVERS_INCLUDE_DHT22_H_

#include <rtthread.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DHT22_START_LOW_US    1000
#define DHT22_TIMEOUT_US      200
#define DHT22_BIT_THRESHOLD   40   /* >40us high = bit 1 */
#define DHT22_MIN_INTERVAL_MS 2000

struct dht22_device
{
    rt_base_t data_pin;
};

rt_err_t dht22_init(struct dht22_device *dev, rt_base_t data_pin);
rt_err_t dht22_read(struct dht22_device *dev, int16_t *temp_x10, uint16_t *humidity_x10);

/* Pure decoder — host-testable, no GPIO/RT-Thread dependency.
 * Returns 0 on checksum OK, -1 on checksum failure. Outputs are only
 * written when the checksum is valid. */
int dht22_decode_frame(const unsigned char raw_frame[5],
                       short *temp_x10,
                       unsigned short *humidity_x10);

#ifdef __cplusplus
}
#endif
#endif
