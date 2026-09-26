#include "tmp105.h"
#include <stdint.h>
#include <string.h>
#include <errno.h>
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

int tmp105_read_temperature(struct tmp105_dev *dev, int32_t *raw)
{
    if (!dev || !raw)
        return -EINVAL;

    up_mdelay(220);

    struct i2c_config_s config;
    config.frequency = 400000;
    config.address = dev->addr;
    config.addrlen = 7;

    uint8_t cmd = TMP105_PTR_TEMP;
    int ret = i2c_writeread(dev->bus, &config, &cmd, 1, (uint8_t *)raw, 2);
    if (ret < 0)
        return -EIO;

    uint16_t temp_raw = ((uint16_t)((uint8_t *)raw)[0] << 8) | ((uint8_t *)raw)[1];
    int16_t temp_signed = (int16_t)(temp_raw >> 4);
    if (temp_signed & 0x0800)
        temp_signed |= 0xF000;
    *raw = ((int32_t)temp_signed * 625) / 10;

    return 0;
}