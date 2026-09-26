#include "tmp421.h"
#include <rtdevice.h>
#include <stdint.h>

#define TMP421_ADDR 0x2A
#define REG_LOCAL_HIGH 0x00
#define REG_REMOTE1_HIGH 0x01
#define REG_STATUS 0x08
#define REG_LOCAL_LOW 0x10
#define REG_REMOTE1_LOW 0x11
#define REG_MANUFACTURER_ID 0xFE
#define REG_DEVICE_ID 0xFF

static int tmp421_write_then_read(struct tmp421_device *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct rt_i2c_msg msgs[2];
    uint8_t reg_buf = reg;

    msgs[0].addr = dev->i2c_addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].len = 1;
    msgs[0].buf = &reg_buf;

    msgs[1].addr = dev->i2c_addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].len = len;
    msgs[1].buf = buf;

    if (rt_i2c_transfer(dev->bus, msgs, 2) != 2)
        return -1;
    return 0;
}

static int tmp421_read_reg(struct tmp421_device *dev, uint8_t reg, uint8_t *val)
{
    return tmp421_write_then_read(dev, reg, val, 1);
}

int tmp421_init(struct tmp421_device *dev, struct rt_i2c_bus_device *bus)
{
    uint8_t id;
    dev->bus = bus;
    dev->i2c_addr = TMP421_ADDR;

    /* Probe manufacturer ID */
    if (tmp421_read_reg(dev, REG_MANUFACTURER_ID, &id) != 0)
        return -1;
    if (id != 0x55)
        return -1;

    /* Probe device ID */
    if (tmp421_read_reg(dev, REG_DEVICE_ID, &id) != 0)
        return -1;
    if (id != 0x21)
        return -1;

    return 0;
}

static int tmp421_read_temperature(struct tmp421_device *dev, uint8_t high_reg, uint8_t low_reg, int32_t *temp_milli)
{
    uint8_t high, low;
    int16_t raw;

    /* Poll BUSY bit until conversion complete */
    uint8_t status;
    int timeout = 130;
    do {
        if (tmp421_read_reg(dev, REG_STATUS, &status) != 0)
            return -1;
        if (!(status & 0x80))
            break;
        rt_thread_mdelay(1);
    } while (--timeout > 0);
    if (timeout == 0)
        return -1;

    /* Read high byte */
    if (tmp421_read_reg(dev, high_reg, &high) != 0)
        return -1;

    /* Read low byte */
    if (tmp421_read_reg(dev, low_reg, &low) != 0)
        return -1;

    /* Combine: high byte is signed 8-bit, low byte upper nibble is fractional */
    raw = ((int16_t)(int8_t)high << 4) | ((low >> 4) & 0x0F);
    /* Convert to milli_degC: raw * 625 / 10 */
    *temp_milli = ((int32_t)raw * 625) / 10;
    return 0;
}

int tmp421_read_local(struct tmp421_device *dev, int32_t *temp_milli)
{
    return tmp421_read_temperature(dev, REG_LOCAL_HIGH, REG_LOCAL_LOW, temp_milli);
}

int tmp421_read_remote(struct tmp421_device *dev, int32_t *temp_milli)
{
    return tmp421_read_temperature(dev, REG_REMOTE1_HIGH, REG_REMOTE1_LOW, temp_milli);
}
