#include "dht22_ref.h"

int dht22_init(struct dht22_device *dev, int data_pin)
{
    if (dev == 0) return -1;
    dev->data_pin = data_pin;
    return hal_gpio_init_out(data_pin, 1);
}

static int dht22_wait_level(struct dht22_device *dev, int level, int max_us)
{
    int us = 0;
    while (hal_gpio_read(dev->data_pin) == level) {
        os_cputime_delay_usecs(1);
        if (++us > max_us) return -1;
    }
    return us;
}

int dht22_read(struct dht22_device *dev, int16_t *temp_x10, uint16_t *humidity_x10)
{
    uint8_t data[5] = {0};
    if (dev == 0 || temp_x10 == 0 || humidity_x10 == 0) return -1;
    hal_gpio_init_out(dev->data_pin, 1);
    hal_gpio_write(dev->data_pin, 0);
    os_time_delay(2);
    hal_gpio_write(dev->data_pin, 1);
    os_cputime_delay_usecs(30);
    hal_gpio_init_in(dev->data_pin, 0);

    if (dht22_wait_level(dev, 0, DHT22_TIMEOUT_US) < 0) return -1;
    if (dht22_wait_level(dev, 1, DHT22_TIMEOUT_US) < 0) return -1;
    for (int i = 0; i < 40; i++) {
        int high_us;
        if (dht22_wait_level(dev, 0, DHT22_TIMEOUT_US) < 0) return -1;
        high_us = dht22_wait_level(dev, 1, DHT22_TIMEOUT_US);
        if (high_us < 0) return -1;
        data[i / 8] <<= 1;
        if ((uint32_t)high_us > DHT22_BIT_THRESHOLD) data[i / 8] |= 1U;
    }
    return dht22_decode_frame((const unsigned char *)data,
                              (short *)temp_x10,
                              (unsigned short *)humidity_x10);
}

int dht22_decode_frame(const unsigned char raw_frame[5], short *temp_x10,
                       unsigned short *humidity_x10)
{
    unsigned char sum;
    unsigned short raw_t;
    if (raw_frame == 0 || temp_x10 == 0 || humidity_x10 == 0) return -1;
    sum = (unsigned char)(raw_frame[0] + raw_frame[1] + raw_frame[2] + raw_frame[3]);
    if (sum != raw_frame[4]) return -1;
    *humidity_x10 = (unsigned short)(((unsigned short)raw_frame[0] << 8) | raw_frame[1]);
    raw_t = (unsigned short)(((unsigned short)raw_frame[2] << 8) | raw_frame[3]);
    *temp_x10 = (raw_t & 0x8000U) ? -(short)(raw_t & 0x7FFFU) : (short)raw_t;
    return 0;
}
