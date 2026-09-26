#include "tmp421.h"
#include <stdint.h>

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
    rc = hal_i2c_master_write(dev->i2c_num, &data, OS_TIMEOUT_NEVER, 1);
    if (rc != 0) return -1;

    data.address = dev->addr << 1;
    data.len = len;
    data.buffer = buf;
    rc = hal_i2c_master_read(dev->i2c_num, &data, OS_TIMEOUT_NEVER, 1);
    if (rc != 0) return -1;

    return 0;
}

static int tmp421_write(struct tmp421_dev *dev, uint8_t reg, uint8_t val)
{
    struct hal_i2c_master_data data;
    uint8_t buf[2] = {reg, val};
    data.address = dev->addr << 1;
    data.len = 2;
    data.buffer = buf;
    return hal_i2c_master_write(dev->i2c_num, &data, OS_TIMEOUT_NEVER, 1);
}

static int tmp421_read_reg(struct tmp421_dev *dev, uint8_t reg, uint8_t *val)
{
    return tmp421_write_then_read(dev, reg, val, 1);
}

static int tmp421_poll_busy(struct tmp421_dev *dev)
{
    uint8_t status;
    int retries = 10;
    while (retries--) {
        if (tmp421_read_reg(dev, TMP421_REG_STATUS, &status) != 0)
            return -1;
        if (!(status & TMP421_BUSY_BIT))
            return 0;
        os_time_delay(15);
    }
    return -1;
}

static int tmp421_read_temp(struct tmp421_dev *dev, uint8_t high_reg, uint8_t low_reg, int32_t *temp)
{
    uint8_t high, low;
    int16_t raw;
    int32_t milli;

    if (tmp421_poll_busy(dev) != 0)
        return -1;

    if (tmp421_read_reg(dev, high_reg, &high) != 0)
        return -1;
    if (tmp421_read_reg(dev, low_reg, &low) != 0)
        return -1;

    raw = ((int16_t)((int8_t)high) << 4) | (low >> 4);
    milli = ((int32_t)raw * 625) / 10;
    *temp = milli;
    return 0;
}

int tmp421_init(struct tmp421_dev *dev, void *bus_handle)
{
    uint8_t id;
    dev->i2c_num = (uint8_t)(uintptr_t)bus_handle;
    dev->addr = TMP421_ADDR;

    if (tmp421_read_reg(dev, TMP421_REG_MANUFACTURER_ID, &id) != 0)
        return -1;
    if (id != 0x55)
        return -1;

    if (tmp421_read_reg(dev, TMP421_REG_DEVICE_ID, &id) != 0)
        return -1;
    if (id != 0x21)
        return -1;

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
