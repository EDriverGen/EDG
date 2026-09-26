#include "ds18b20.h"
#include "gpio_if.h"
#include "hdf_base.h"
#include "osal_time.h"
#include <stdint.h>

#define DS18B20_GPIO_PIN 1

static int32_t ds18b20_reset(struct ds18b20_dev *dev)
{
    int32_t ret;
    uint16_t val;
    // Pull low for at least 480 us
    ret = GpioSetDir(dev->gpio_pin, 0); // output
    if (ret != HDF_SUCCESS) return ret;
    ret = GpioWrite(dev->gpio_pin, 0);
    if (ret != HDF_SUCCESS) return ret;
    OsalUDelay(480);
    // Release and wait for presence pulse
    ret = GpioSetDir(dev->gpio_pin, 1); // input
    if (ret != HDF_SUCCESS) return ret;
    OsalUDelay(70); // wait for presence pulse start
    ret = GpioRead(dev->gpio_pin, &val);
    if (ret != HDF_SUCCESS) return ret;
    if (val != 0) return HDF_FAILURE; // no presence pulse
    OsalUDelay(240);
    ret = GpioRead(dev->gpio_pin, &val);
    if (ret != HDF_SUCCESS) return ret;
    if (val != 1) return HDF_FAILURE; // presence pulse too long
    return HDF_SUCCESS;
}

static int32_t ds18b20_write_byte(struct ds18b20_dev *dev, uint8_t byte)
{
    int32_t ret;
    for (int i = 0; i < 8; i++) {
        if (byte & (1 << i)) {
            // write 1
            ret = GpioSetDir(dev->gpio_pin, 0);
            if (ret != HDF_SUCCESS) return ret;
            ret = GpioWrite(dev->gpio_pin, 0);
            if (ret != HDF_SUCCESS) return ret;
            OsalUDelay(1);
            ret = GpioSetDir(dev->gpio_pin, 1);
            if (ret != HDF_SUCCESS) return ret;
            OsalUDelay(60);
        } else {
            // write 0
            ret = GpioSetDir(dev->gpio_pin, 0);
            if (ret != HDF_SUCCESS) return ret;
            ret = GpioWrite(dev->gpio_pin, 0);
            if (ret != HDF_SUCCESS) return ret;
            OsalUDelay(60);
            ret = GpioSetDir(dev->gpio_pin, 1);
            if (ret != HDF_SUCCESS) return ret;
            OsalUDelay(1);
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
        ret = GpioSetDir(dev->gpio_pin, 0);
        if (ret != HDF_SUCCESS) return ret;
        ret = GpioWrite(dev->gpio_pin, 0);
        if (ret != HDF_SUCCESS) return ret;
        OsalUDelay(1);
        ret = GpioSetDir(dev->gpio_pin, 1);
        if (ret != HDF_SUCCESS) return ret;
        OsalUDelay(1);
        ret = GpioRead(dev->gpio_pin, &val);
        if (ret != HDF_SUCCESS) return ret;
        if (val) *byte |= (1 << i);
        OsalUDelay(60);
    }
    return HDF_SUCCESS;
}

int32_t ds18b20_init(struct ds18b20_dev *dev, void *bus_handle)
{
    (void)bus_handle;
    dev->gpio_pin = DS18B20_GPIO_PIN;
    return ds18b20_reset(dev);
}

int32_t ds18b20_read_temperature(struct ds18b20_dev *dev, int32_t *raw)
{
    int32_t ret;
    uint8_t lsb, msb;
    int16_t raw16;
    // Skip ROM (single device)
    ret = ds18b20_write_byte(dev, 0xCC);
    if (ret != HDF_SUCCESS) return ret;
    // Convert T
    ret = ds18b20_write_byte(dev, 0x44);
    if (ret != HDF_SUCCESS) return ret;
    // Wait for conversion (12-bit: 750ms)
    OsalMSleep(750);
    // Reset
    ret = ds18b20_reset(dev);
    if (ret != HDF_SUCCESS) return ret;
    // Skip ROM
    ret = ds18b20_write_byte(dev, 0xCC);
    if (ret != HDF_SUCCESS) return ret;
    // Read scratchpad
    ret = ds18b20_write_byte(dev, 0xBE);
    if (ret != HDF_SUCCESS) return ret;
    ret = ds18b20_read_byte(dev, &lsb);
    if (ret != HDF_SUCCESS) return ret;
    ret = ds18b20_read_byte(dev, &msb);
    if (ret != HDF_SUCCESS) return ret;
    raw16 = (int16_t)((msb << 8) | lsb);
    // Sign extend from bit 11 (12-bit signed)
    if (raw16 & 0x0800) {
        raw16 |= 0xF000;
    }
    // Convert to milli-degrees: raw16 * 0.0625 * 1000 = raw16 * 625 / 10
    *raw = (int32_t)((int64_t)raw16 * 625 / 10);
    return HDF_SUCCESS;
}