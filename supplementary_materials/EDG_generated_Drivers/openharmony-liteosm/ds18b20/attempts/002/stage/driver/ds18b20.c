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
    OsalMDelay(1); // >480 us
    // Release and wait for presence pulse
    ret = GpioSetDir(dev->gpio_pin, 1); // input
    if (ret != HDF_SUCCESS) return ret;
    OsalUDelay(70); // wait 15-60 us for presence low
    ret = GpioRead(dev->gpio_pin, &val);
    if (ret != HDF_SUCCESS) return ret;
    if (val != 0) return HDF_FAILURE; // no presence
    // Wait for presence pulse to end (max 240 us)
    uint32_t timeout = 0;
    while (timeout < 300) {
        ret = GpioRead(dev->gpio_pin, &val);
        if (ret != HDF_SUCCESS) return ret;
        if (val == 1) break;
        OsalUDelay(1);
        timeout++;
    }
    if (timeout >= 300) return HDF_FAILURE;
    return HDF_SUCCESS;
}

static int32_t ds18b20_write_byte(struct ds18b20_dev *dev, uint8_t byte)
{
    int32_t ret;
    for (int i = 0; i < 8; i++) {
        uint8_t bit = (byte >> i) & 1;
        // Pull low
        ret = GpioSetDir(dev->gpio_pin, 0);
        if (ret != HDF_SUCCESS) return ret;
        ret = GpioWrite(dev->gpio_pin, 0);
        if (ret != HDF_SUCCESS) return ret;
        if (bit) {
            OsalUDelay(5); // write 1 low time <15 us
            ret = GpioSetDir(dev->gpio_pin, 1); // release
            if (ret != HDF_SUCCESS) return ret;
            OsalUDelay(65); // complete slot
        } else {
            OsalUDelay(60); // write 0 low time ~60 us
            ret = GpioSetDir(dev->gpio_pin, 1); // release
            if (ret != HDF_SUCCESS) return ret;
            OsalUDelay(5); // recovery
        }
    }
    return HDF_SUCCESS;
}

static int32_t ds18b20_read_byte(struct ds18b20_dev *dev, uint8_t *byte)
{
    int32_t ret;
    uint8_t result = 0;
    for (int i = 0; i < 8; i++) {
        uint16_t val;
        // Generate read slot
        ret = GpioSetDir(dev->gpio_pin, 0);
        if (ret != HDF_SUCCESS) return ret;
        ret = GpioWrite(dev->gpio_pin, 0);
        if (ret != HDF_SUCCESS) return ret;
        OsalUDelay(2); // hold low >1 us
        ret = GpioSetDir(dev->gpio_pin, 1); // release
        if (ret != HDF_SUCCESS) return ret;
        OsalUDelay(5); // wait for data
        ret = GpioRead(dev->gpio_pin, &val);
        if (ret != HDF_SUCCESS) return ret;
        if (val) result |= (1 << i);
        OsalUDelay(60); // complete slot
    }
    *byte = result;
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
    OsalMDelay(750);
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
    
    raw16 = (int16_t)((uint16_t)msb << 8 | lsb);
    // Sign extend from 12 bits
    if (raw16 & 0x0800) {
        raw16 |= 0xF000;
    }
    // Convert to milli-degrees: raw * 0.0625 * 1000 = raw * 625 / 10
    *raw = (int32_t)raw16 * 625 / 10;
    return HDF_SUCCESS;
}