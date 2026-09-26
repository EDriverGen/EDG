#include "tmp105.h"
#include <arch.h>
#include <errno.h>
#include <stdint.h>

#define TMP105_ADDR 0x48
#define TMP105_PTR_TEMP 0x00

int tmp105_init(struct tmp105_dev *dev, struct i2c_master_s *bus)
{
    if (!dev || !bus)
        return -EINVAL;
    dev->bus = bus;
    dev->addr = TMP105_ADDR;
    return 0;
}

static int tmp105_write_then_read(struct tmp105_dev *dev, uint8_t reg, uint8_t *buf, int len)
{
    struct i2c_config_s config;
    config.frequency = 400000;
    config.address = dev->addr;
    config.addrlen = 7;

    int ret = I2C_WRITEREAD(dev->bus, &config, &reg, 1, buf, len);
    if (ret < 0)
        return -EIO;
    return 0;
}

int tmp105_read_temperature(struct tmp105_dev *dev, int32_t *raw)
{
    if (!dev || !raw)
        return -EINVAL;

    up_mdelay(220);

    uint8_t buf[2];
    int ret = tmp105_write_then_read(dev, TMP105_PTR_TEMP, buf, 2);
    if (ret < 0)
        return ret;

    uint16_t raw16 = ((uint16_t)buf[0] << 8) | buf[1];
    int16_t temp12 = (int16_t)(raw16 >> 4);
    if (temp12 & 0x0800)
        temp12 |= 0xF000;

    *raw = ((int32_t)temp12 * 125) / 2;
    return 0;
}