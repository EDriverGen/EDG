#include "tmp421.h"
#include <assert.h>
#include <stddef.h>

#include <hal/hal_i2c.h>
#include <os/os_time.h>
#define TMP421_ADDR 0x2A
#define TMP421_REG_STATUS 0x08
#define TMP421_REG_LOCAL_HIGH 0x00
#define TMP421_REG_LOCAL_LOW 0x10
#define TMP421_REG_REMOTE_HIGH 0x01
#define TMP421_REG_REMOTE_LOW 0x11
#define TMP421_REG_MANUFACTURER_ID 0xFE
#define TMP421_REG_DEVICE_ID 0xFF
#define TMP421_BUSY_BIT 0x80
#define TMP421_CONVERSION_DELAY_MS 130

static int tmp421_write_then_read(struct tmp421_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct hal_i2c_master_data data;
    int rc;

    data.address = dev->addr << 1;
    data.len = 1;
    data.buffer = &reg;
    rc = hal_i2c_master_write(dev->i2c_num, &data, OS_TIME_FOREVER, 1);
    if (rc != 0) {
        return -1;
    }

    data.address = dev->addr << 1;
    data.len = len;
    data.buffer = buf;
    rc = hal_i2c_master_read(dev->i2c_num, &data, OS_TIME_FOREVER, 1);
    if (rc != 0) {
        return -1;
    }

    return 0;
}

static int tmp421_poll_busy(struct tmp421_dev *dev)
{
    uint8_t status;
    int retries = 10;
    while (retries--) {
        if (tmp421_write_then_read(dev, TMP421_REG_STATUS, &status, 1) != 0) {
            return -1;
        }
        if (!(status & TMP421_BUSY_BIT)) {
            return 0;
        }
        os_time_delay(OS_TICKS_PER_SEC / 100); // 10 ms
    }
    return -1;
}

static int tmp421_read_temp(struct tmp421_dev *dev, uint8_t high_reg, uint8_t low_reg, int32_t *temp)
{
    uint8_t high_byte, low_byte;
    int16_t raw;
    int32_t milli;

    if (tmp421_poll_busy(dev) != 0) {
        return -1;
    }

    if (tmp421_write_then_read(dev, high_reg, &high_byte, 1) != 0) {
        return -1;
    }
    if (tmp421_write_then_read(dev, low_reg, &low_byte, 1) != 0) {
        return -1;
    }

    // Combine: high_byte is signed 8-bit, low_byte upper nibble is fractional
    raw = ((int16_t)(int8_t)high_byte << 4) | (low_byte >> 4);
    // Convert to milli_degC: raw * 625 / 10
    milli = ((int32_t)raw * 625) / 10;
    *temp = milli;
    return 0;
}

int tmp421_init(struct tmp421_dev *dev, void *bus_handle)
{
    uint8_t id;
    int rc;

    assert(dev != NULL);
    assert(bus_handle != NULL);

    dev->i2c_num = (uint8_t)(uintptr_t)bus_handle;
    dev->addr = TMP421_ADDR;

    // Probe manufacturer ID
    rc = tmp421_write_then_read(dev, TMP421_REG_MANUFACTURER_ID, &id, 1);
    if (rc != 0 || id != 0x55) {
        return -1;
    }

    // Probe device ID
    rc = tmp421_write_then_read(dev, TMP421_REG_DEVICE_ID, &id, 1);
    if (rc != 0 || id != 0x21) {
        return -1;
    }

    return 0;
}

int tmp421_read_temp_local(struct tmp421_dev *dev, int32_t *temp_local)
{
    return tmp421_read_temp(dev, TMP421_REG_LOCAL_HIGH, TMP421_REG_LOCAL_LOW, temp_local);
}

int tmp421_read_temp_remote(struct tmp421_dev *dev, int32_t *temp_remote)
{
    return tmp421_read_temp(dev, TMP421_REG_REMOTE_HIGH, TMP421_REG_REMOTE_LOW, temp_remote);
}
