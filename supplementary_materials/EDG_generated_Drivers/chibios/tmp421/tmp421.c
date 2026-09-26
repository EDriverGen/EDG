#include "tmp421.h"
#include <stddef.h>
#include <string.h>

#include "hal_i2c.h"
#define TMP421_STATUS_REG 0x08
#define TMP421_LOCAL_HIGH 0x00
#define TMP421_LOCAL_LOW  0x10
#define TMP421_REMOTE_HIGH 0x01
#define TMP421_REMOTE_LOW  0x11
#define TMP421_MANUFACTURER_ID 0xFE
#define TMP421_DEVICE_ID 0xFF
#define TMP421_BUSY_BIT 0x80
#define TMP421_CONVERSION_TIMEOUT_MS 130
#define TMP421_I2C_TIMEOUT_MS 100

static int tmp421_write_then_read(struct tmp421_device *dev, uint8_t reg, uint8_t *rxbuf, size_t rxlen)
{
    msg_t ret;
    i2cAcquireBus(dev->bus_handle);
    ret = i2cMasterTransmitTimeout(dev->bus_handle, dev->i2c_addr, &reg, 1, rxbuf, rxlen, MS2ST(TMP421_I2C_TIMEOUT_MS));
    i2cReleaseBus(dev->bus_handle);
    return (ret == MSG_OK) ? 0 : -1;
}

static int tmp421_read_reg(struct tmp421_device *dev, uint8_t reg, uint8_t *val)
{
    return tmp421_write_then_read(dev, reg, val, 1);
}

static int tmp421_poll_busy(struct tmp421_device *dev)
{
    uint8_t status;
    int timeout = TMP421_CONVERSION_TIMEOUT_MS;
    while (timeout > 0) {
        if (tmp421_read_reg(dev, TMP421_STATUS_REG, &status) != 0)
            return -1;
        if (!(status & TMP421_BUSY_BIT))
            return 0;
        chThdSleepMilliseconds(1);
        timeout--;
    }
    return -1;
}

static int tmp421_read_temperature(struct tmp421_device *dev, uint8_t high_reg, uint8_t low_reg, int32_t *temp)
{
    uint8_t high_byte, low_byte;
    int16_t combined;
    int32_t result;

    if (tmp421_poll_busy(dev) != 0)
        return -1;

    if (tmp421_write_then_read(dev, high_reg, &high_byte, 1) != 0)
        return -1;
    if (tmp421_write_then_read(dev, low_reg, &low_byte, 1) != 0)
        return -1;

    combined = ((int16_t)((int8_t)high_byte) << 4) | (low_byte >> 4);
    result = (int32_t)combined * 625 / 10;
    *temp = result;
    return 0;
}

int tmp421_init(struct tmp421_device *dev, void *bus_handle)
{
    uint8_t val;
    dev->bus_handle = (I2CDriver *)bus_handle;
    dev->i2c_addr = TMP421_I2C_ADDR;

    if (tmp421_read_reg(dev, TMP421_MANUFACTURER_ID, &val) != 0)
        return -1;
    if (val != 0x55)
        return -1;

    if (tmp421_read_reg(dev, TMP421_DEVICE_ID, &val) != 0)
        return -1;
    if (val != 0x21)
        return -1;

    return 0;
}

int tmp421_read_local(struct tmp421_device *dev, int32_t *temp_local_val)
{
    return tmp421_read_temperature(dev, TMP421_LOCAL_HIGH, TMP421_LOCAL_LOW, temp_local_val);
}

int tmp421_read_remote(struct tmp421_device *dev, int32_t *temp_remote_val)
{
    return tmp421_read_temperature(dev, TMP421_REMOTE_HIGH, TMP421_REMOTE_LOW, temp_remote_val);
}
