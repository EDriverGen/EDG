#include "bh1750.h"
#include <nuttx.h>
#include <stdint.h>

#define BH1750_ADDR 0x23
#define BH1750_CMD_POWER_ON 0x01
#define BH1750_CMD_CONT_HRES 0x10
#define BH1750_MEAS_DELAY_MS 180

int bh1750_init(struct bh1750_dev_s *dev, struct i2c_master_s *bus)
{
    struct i2c_config_s config;
    uint8_t cmd = BH1750_CMD_POWER_ON;
    int ret;

    dev->bus = bus;
    dev->addr = BH1750_ADDR;

    config.frequency = 400000;
    config.address = BH1750_ADDR;
    config.addrlen = 7;

    ret = i2c_write(bus, &config, &cmd, 1);
    if (ret < 0) {
        return -EIO;
    }

    return 0;
}

int bh1750_read_illuminance(struct bh1750_dev_s *dev, int32_t *raw)
{
    struct i2c_config_s config;
    uint8_t cmd = BH1750_CMD_CONT_HRES;
    uint8_t buf[2];
    int ret;

    config.frequency = 400000;
    config.address = BH1750_ADDR;
    config.addrlen = 7;

    ret = i2c_write(dev->bus, &config, &cmd, 1);
    if (ret < 0) {
        return -EIO;
    }

    up_mdelay(BH1750_MEAS_DELAY_MS);

    ret = i2c_read(dev->bus, &config, buf, 2);
    if (ret < 0) {
        return -EIO;
    }

    *raw = ((int32_t)buf[0] << 8) | buf[1];
    return 0;
}
