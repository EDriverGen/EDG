/*
 * DHT22 sensor driver for ChibiOS (GPIO)
 */
#include "dht22_ref.h"

static int dht22_wait(ioportid_t port, ioportmask_t pad, int level, int max_us)
{
    int us = 0;
    while (palReadPad(port, pad) == (uint8_t)level) {
        chSysPolledDelayX(US2RTC(STM32_HCLK, 1));
        if (++us > max_us) return -1;
    }
    return us;
}

int dht22_init(struct dht22_device *dev, ioportid_t port, ioportmask_t pad)
{
    if (!dev) return -1;
    dev->port = port; dev->pad = pad;
    palSetPadMode(port, pad, PAL_MODE_OUTPUT_PUSHPULL);
    palSetPad(port, pad);
    return 0;
}

int dht22_read(struct dht22_device *dev, int16_t *temp_x10, uint16_t *humidity_x10)
{
    uint8_t data[5] = {0};
    if (!dev || !temp_x10 || !humidity_x10) return -1;

    palSetPadMode(dev->port, dev->pad, PAL_MODE_OUTPUT_PUSHPULL);
    palClearPad(dev->port, dev->pad);
    chThdSleepMilliseconds(2);
    palSetPad(dev->port, dev->pad);
    chSysPolledDelayX(US2RTC(STM32_HCLK, 30));
    palSetPadMode(dev->port, dev->pad, PAL_MODE_INPUT_PULLUP);

    if (dht22_wait(dev->port, dev->pad, PAL_LOW, DHT22_TIMEOUT_US) < 0) return -1;
    if (dht22_wait(dev->port, dev->pad, PAL_HIGH, DHT22_TIMEOUT_US) < 0) return -1;

    for (int i = 0; i < 40; i++) {
        if (dht22_wait(dev->port, dev->pad, PAL_LOW, DHT22_TIMEOUT_US) < 0) return -1;
        int high = dht22_wait(dev->port, dev->pad, PAL_HIGH, DHT22_TIMEOUT_US);
        if (high < 0) return -1;
        data[i/8] <<= 1;
        if (high > DHT22_BIT_THRESHOLD) data[i/8] |= 1;
    }

    /* verify checksum */
    uint8_t sum = (uint8_t)(data[0] + data[1] + data[2] + data[3]);
    if (sum != data[4]) return -1;
    /* humidity: data[0..1], 0.1% RH */
    *humidity_x10 = (uint16_t)((uint16_t)data[0] << 8 | data[1]);
    /* temperature: data[2..3], 0.1 degC, bit15=sign */
    uint16_t raw_t = (uint16_t)((uint16_t)data[2] << 8 | data[3]);
    if (raw_t & 0x8000)
        *temp_x10 = -(int16_t)(raw_t & 0x7FFF);
    else
        *temp_x10 = (int16_t)raw_t;
    return 0;
}
