#include "tmp421.h"
#include "xtimer.h"
#include <stdint.h>
#include <stddef.h>

#include "riot.h"
#define TMP421_REG_LOCAL_HIGH 0x00
#define TMP421_REG_LOCAL_LOW  0x10
#define TMP421_REG_REMOTE_HIGH 0x01
#define TMP421_REG_REMOTE_LOW  0x11
#define TMP421_REG_STATUS     0x08
#define TMP421_REG_MANUF_ID   0xFE
#define TMP421_REG_DEVICE_ID  0xFF
#define TMP421_BUSY_BIT       0x80
#define TMP421_CONV_DELAY_MS  130

static int tmp421_read_reg(struct tmp421_device *dev, uint8_t reg, uint8_t *buf, size_t len)
{
    return i2c_read_regs(dev->bus, dev->addr, reg, buf, len, 0);
}

static int tmp421_write_reg(struct tmp421_device *dev, uint8_t reg, uint8_t val)
{
    uint8_t data[2] = {reg, val};
    return i2c_write_bytes(dev->bus, dev->addr, data, 2, 0);
}

static int tmp421_poll_busy(struct tmp421_device *dev)
{
    uint8_t status;
    int ret;
    for (int i = 0; i < 10; i++) {
        ret = tmp421_read_reg(dev, TMP421_REG_STATUS, &status, 1);
        if (ret != 0) return ret;
        if (!(status & TMP421_BUSY_BIT)) return 0;
        xtimer_msleep(15);
    }
    return -1;
}

static int tmp421_read_temperature(struct tmp421_device *dev, uint8_t high_reg, uint8_t low_reg, int32_t *temp)
{
    int ret;
    uint8_t high_byte, low_byte;
    
    ret = tmp421_poll_busy(dev);
    if (ret != 0) return ret;
    
    ret = tmp421_read_reg(dev, high_reg, &high_byte, 1);
    if (ret != 0) return ret;
    
    ret = tmp421_read_reg(dev, low_reg, &low_byte, 1);
    if (ret != 0) return ret;
    
    int16_t raw = ((int16_t)((int8_t)high_byte) << 4) | (low_byte >> 4);
    *temp = ((int32_t)raw * 625) / 10;
    return 0;
}

int tmp421_init(struct tmp421_device *dev, i2c_t bus, uint8_t addr)
{
    dev->bus = bus;
    dev->addr = addr;
    
    uint8_t id;
    int ret;
    
    ret = tmp421_read_reg(dev, TMP421_REG_MANUF_ID, &id, 1);
    if (ret != 0 || id != 0x55) return -1;
    
    ret = tmp421_read_reg(dev, TMP421_REG_DEVICE_ID, &id, 1);
    if (ret != 0 || id != 0x21) return -1;
    
    return 0;
}

int tmp421_read_local(struct tmp421_device *dev, int32_t *temp_local_val)
{
    return tmp421_read_temperature(dev, TMP421_REG_LOCAL_HIGH, TMP421_REG_LOCAL_LOW, temp_local_val);
}

int tmp421_read_remote(struct tmp421_device *dev, int32_t *temp_remote_val)
{
    return tmp421_read_temperature(dev, TMP421_REG_REMOTE_HIGH, TMP421_REG_REMOTE_LOW, temp_remote_val);
}
