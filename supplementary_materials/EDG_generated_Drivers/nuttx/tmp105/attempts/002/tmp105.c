#include "tmp105.h"
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include "arch.h"

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

int tmp105_read_temperature(struct tmp105_dev *dev, int32_t *temp)
{
    struct i2c_config_s config;
    uint8_t cmd = TMP105_PTR_TEMP;
    uint8_t buf[2];
    int ret;

    if (!dev || !dev->bus || !temp)
        return -EINVAL;

    up_mdelay(220);

    config.frequency = 400000;
    config.address = dev->addr;
    config.addrlen = 7;

    ret = I2C_WRITEREAD(dev->bus, &config, &cmd, 1, buf, 2);
    if (ret < 0)
        return -EIO;

    int16_t raw = (int16_t)((buf[0] << 8) | buf[1]);
    raw >>= 4;
    if (raw & 0x0800)
        raw |= 0xF000;
    *temp = ((int32_t)raw * 125) / 2;
    return 0;
}