#include "ds18b20.h"
#include "gpio_if.h"
#include "hdf_base.h"
#include "osal_time.h"
#include <stdint.h>

#define DS18B20_GPIO_PIN 1

static int32_t ds18b20_reset_pulse(struct ds18b20_dev *dev)
{
    int32_t ret;
    uint16_t val;
    ret = GpioSetDir(dev->gpio_pin, 1);
    if (ret != HDF_SUCCESS) return HDF_FAILURE;
    ret = GpioWrite(dev->gpio_pin, 0);
    if (ret != HDF_SUCCESS) return HDF_FAILURE;
    OsalUDelay(480);
    ret = GpioSetDir(dev->gpio_pin, 0);
    if (ret != HDF_SUCCESS) return HDF_FAILURE;
    OsalUDelay(70);
    ret = GpioRead(dev->gpio_pin, &val);
    if (ret != HDF_SUCCESS) return HDF_FAILURE;
    if (val != 0) return HDF_FAILURE;
    OsalUDelay(410);
    return HDF_SUCCESS;
}

static int32_t ds18b20_write_byte(struct ds18b20_dev *dev, uint8_t byte)
{
    int32_t ret;
    for (int i = 0; i < 8; i++) {
        ret = GpioSetDir(dev->gpio_pin, 1);
        if (ret != HDF_SUCCESS) return HDF_FAILURE;
        ret = GpioWrite(dev->gpio_pin, 0);
        if (ret != HDF_SUCCESS) return HDF_FAILURE;
        if (byte & (1 << i)) {
            OsalUDelay(5);
            ret = GpioSetDir(dev->gpio_pin, 0);
            if (ret != HDF_SUCCESS) return HDF_FAILURE;
            OsalUDelay(55);
        } else {
            OsalUDelay(55);
            ret = GpioSetDir(dev->gpio_pin, 0);
            if (ret != HDF_SUCCESS) return HDF_FAILURE;
            OsalUDelay(5);
        }
    }
    return HDF_SUCCESS;
}

static int32_t ds18b20_read_byte(struct ds18b20_dev *dev, uint8_t *byte)
{
    int32_t ret;
    uint16_t val;
    *byte = 0;
    for (int i = 0; i < 8; i++) {
        ret = GpioSetDir(dev->gpio_pin, 1);
        if (ret != HDF_SUCCESS) return HDF_FAILURE;
        ret = GpioWrite(dev->gpio_pin, 0);
        if (ret != HDF_SUCCESS) return HDF_FAILURE;
        OsalUDelay(1);
        ret = GpioSetDir(dev->gpio_pin, 0);
        if (ret != HDF_SUCCESS) return HDF_FAILURE;
        OsalUDelay(1);
        ret = GpioRead(dev->gpio_pin, &val);
        if (ret != HDF_SUCCESS) return HDF_FAILURE;
        if (val) *byte |= (1 << i);
        OsalUDelay(55);
    }
    return HDF_SUCCESS;
}

int32_t ds18b20_init(struct ds18b20_dev *dev, void *bus_handle)
{
    (void)bus_handle;
    dev->gpio_pin = DS18B20_GPIO_PIN;
    int32_t ret = GpioSetDir(dev->gpio_pin, 1);
    if (ret != HDF_SUCCESS) return HDF_FAILURE;
    ret = GpioWrite(dev->gpio_pin, 1);
    if (ret != HDF_SUCCESS) return HDF_FAILURE;
    ret = ds18b20_reset_pulse(dev);
    if (ret != HDF_SUCCESS) return HDF_FAILURE;
    return HDF_SUCCESS;
}

int32_t ds18b20_read_temperature(struct ds18b20_dev *dev, int32_t *raw)
{
    int32_t ret;
    uint8_t lsb, msb;
    int16_t raw16;
    ret = ds18b20_reset_pulse(dev);
    if (ret != HDF_SUCCESS) return HDF_FAILURE;
    ret = ds18b20_write_byte(dev, 0xCC);
    if (ret != HDF_SUCCESS) return HDF_FAILURE;
    ret = ds18b20_write_byte(dev, 0x44);
    if (ret != HDF_SUCCESS) return HDF_FAILURE;
    OsalMDelay(750);
    ret = ds18b20_reset_pulse(dev);
    if (ret != HDF_SUCCESS) return HDF_FAILURE;
    ret = ds18b20_write_byte(dev, 0xCC);
    if (ret != HDF_SUCCESS) return HDF_FAILURE;
    ret = ds18b20_write_byte(dev, 0xBE);
    if (ret != HDF_SUCCESS) return HDF_FAILURE;
    ret = ds18b20_read_byte(dev, &lsb);
    if (ret != HDF_SUCCESS) return HDF_FAILURE;
    ret = ds18b20_read_byte(dev, &msb);
    if (ret != HDF_SUCCESS) return HDF_FAILURE;
    raw16 = (int16_t)((uint16_t)msb << 8 | lsb);
    *raw = (int32_t)(raw16 * 625 / 10);
    return HDF_SUCCESS;
}