/*
 * DHT22 sensor driver for RT-Thread (GPIO)
 */
#include "dht22_ref.h"

rt_err_t dht22_init(struct dht22_device *dev, rt_base_t data_pin)
{
    if (dev == RT_NULL) return -RT_EINVAL;
    dev->data_pin = data_pin;
    rt_pin_mode(data_pin, PIN_MODE_OUTPUT);
    rt_pin_write(data_pin, PIN_HIGH);
    return RT_EOK;
}

static int dht22_wait_level(struct dht22_device *dev, int level, int max_us)
{
    int us = 0;
    while (rt_pin_read(dev->data_pin) == level) {
        rt_hw_us_delay(1);
        if (++us > max_us) return -1;
    }
    return us;
}

rt_err_t dht22_read(struct dht22_device *dev, int16_t *temp_x10, uint16_t *humidity_x10)
{
    uint8_t data[5] = {0};
    rt_base_t level;
    if (dev == RT_NULL || temp_x10 == RT_NULL || humidity_x10 == RT_NULL)
        return -RT_EINVAL;

    /* start signal: pull low >= 1ms */
    level = rt_hw_interrupt_disable();
    rt_pin_mode(dev->data_pin, PIN_MODE_OUTPUT);
    rt_pin_write(dev->data_pin, PIN_LOW);
    rt_hw_interrupt_enable(level);
    rt_thread_mdelay(2);

    level = rt_hw_interrupt_disable();
    rt_pin_write(dev->data_pin, PIN_HIGH);
    rt_hw_us_delay(30);
    rt_pin_mode(dev->data_pin, PIN_MODE_INPUT);

    /* sensor response: 80us low + 80us high */
    if (dht22_wait_level(dev, PIN_LOW, DHT22_TIMEOUT_US) < 0) goto err;
    if (dht22_wait_level(dev, PIN_HIGH, DHT22_TIMEOUT_US) < 0) goto err;

    /* read 40 bits */
    for (int i = 0; i < 40; i++) {
        if (dht22_wait_level(dev, PIN_LOW, DHT22_TIMEOUT_US) < 0) goto err;
        int high_us = dht22_wait_level(dev, PIN_HIGH, DHT22_TIMEOUT_US);
        if (high_us < 0) goto err;
        data[i / 8] <<= 1;
        if (high_us > DHT22_BIT_THRESHOLD) data[i / 8] |= 1;
    }
    rt_hw_interrupt_enable(level);

    /* verify checksum */
    uint8_t sum = (uint8_t)(data[0] + data[1] + data[2] + data[3]);
    if (sum != data[4]) return -RT_EIO;
    /* Reuse the pure decoder so the bit-bang path and host logic tests
     * exercise the exact same sign-extension and range arithmetic. */
    if (dht22_decode_frame((const unsigned char *)data,
                           temp_x10, humidity_x10) != 0)
        return -RT_EIO;
    return RT_EOK;

err:
    rt_hw_interrupt_enable(level);
    return -RT_EIO;
}

/* Pure-logic decoder shared with host-side logic tests. Validates the
 * checksum, then reconstructs humidity (0.1 %RH) and signed temperature
 * (0.1 °C, sign in bit 15) per AM2302 datasheet. */
int dht22_decode_frame(const unsigned char raw_frame[5],
                       short *temp_x10,
                       unsigned short *humidity_x10)
{
    if (raw_frame == 0 || temp_x10 == 0 || humidity_x10 == 0) return -1;
    unsigned char sum = (unsigned char)(raw_frame[0] + raw_frame[1]
                                      + raw_frame[2] + raw_frame[3]);
    if (sum != raw_frame[4]) return -1;
    *humidity_x10 = (unsigned short)(((unsigned short)raw_frame[0] << 8)
                                     | raw_frame[1]);
    unsigned short raw_t = (unsigned short)(((unsigned short)raw_frame[2] << 8)
                                            | raw_frame[3]);
    if (raw_t & 0x8000)
        *temp_x10 = -(short)(raw_t & 0x7FFF);
    else
        *temp_x10 = (short)raw_t;
    return 0;
}
