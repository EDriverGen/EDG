/*
 * DHT22 sensor driver for OpenHarmony HDF
 */
#include "dht22_ref.h"
#include "hdf_log.h"
#define HDF_LOG_TAG dht22

static int dht22_wait(uint16_t gpio, uint16_t level, int max_us)
{
    int us = 0; uint16_t val;
    while (1) {
        GpioRead(gpio, &val);
        if (val != level) break;
        OsalUDelay(1);
        if (++us > max_us) return -1;
    }
    return us;
}

int32_t dht22_init(struct dht22_device *dev, uint16_t gpio_num)
{
    if (!dev) return HDF_ERR_INVALID_PARAM;
    dev->data_gpio = gpio_num;
    GpioSetDir(gpio_num, GPIO_DIR_OUT);
    GpioWrite(gpio_num, GPIO_VAL_HIGH);
    return HDF_SUCCESS;
}

int32_t dht22_read(struct dht22_device *dev, int16_t *temp_x10, uint16_t *humidity_x10)
{
    uint8_t data[5] = {0};
    if (!dev || !temp_x10 || !humidity_x10) return HDF_ERR_INVALID_PARAM;

    GpioSetDir(dev->data_gpio, GPIO_DIR_OUT);
    GpioWrite(dev->data_gpio, GPIO_VAL_LOW);
    OsalMSleep(2);
    GpioWrite(dev->data_gpio, GPIO_VAL_HIGH);
    OsalUDelay(30);
    GpioSetDir(dev->data_gpio, GPIO_DIR_IN);

    if (dht22_wait(dev->data_gpio, GPIO_VAL_LOW, DHT22_TIMEOUT_US) < 0) return HDF_FAILURE;
    if (dht22_wait(dev->data_gpio, GPIO_VAL_HIGH, DHT22_TIMEOUT_US) < 0) return HDF_FAILURE;

    for (int i = 0; i < 40; i++) {
        if (dht22_wait(dev->data_gpio, GPIO_VAL_LOW, DHT22_TIMEOUT_US) < 0) return HDF_FAILURE;
        int high = dht22_wait(dev->data_gpio, GPIO_VAL_HIGH, DHT22_TIMEOUT_US);
        if (high < 0) return HDF_FAILURE;
        data[i/8] <<= 1;
        if (high > DHT22_BIT_THRESHOLD) data[i/8] |= 1;
    }

    /* verify checksum */
    uint8_t sum = (uint8_t)(data[0] + data[1] + data[2] + data[3]);
    if (sum != data[4]) return HDF_FAILURE;
    /* humidity: data[0..1], 0.1% RH */
    *humidity_x10 = (uint16_t)((uint16_t)data[0] << 8 | data[1]);
    /* temperature: data[2..3], 0.1 degC, bit15=sign */
    uint16_t raw_t = (uint16_t)((uint16_t)data[2] << 8 | data[3]);
    if (raw_t & 0x8000)
        *temp_x10 = -(int16_t)(raw_t & 0x7FFF);
    else
        *temp_x10 = (int16_t)raw_t;
    return HDF_SUCCESS;
}
