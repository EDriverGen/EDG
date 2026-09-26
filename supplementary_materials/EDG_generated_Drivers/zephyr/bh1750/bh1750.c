#include "bh1750.h"
#include "zephyr.h"
#include <stdint.h>

#define BH1750_ADDR 0x23
#define BH1750_CMD_POWER_ON 0x01
#define BH1750_CMD_CONT_HRES 0x10
#define BH1750_MEAS_DELAY_MS 180

int bh1750_init(const struct device *dev)
{
    uint8_t cmd = BH1750_CMD_POWER_ON;
    int ret = i2c_write(dev, &cmd, 1, BH1750_ADDR);
    if (ret != 0) {
        return -1;
    }
    return 0;
}

int bh1750_sample_fetch(const struct device *dev, int32_t *raw)
{
    uint8_t cmd = BH1750_CMD_CONT_HRES;
    int ret = i2c_write(dev, &cmd, 1, BH1750_ADDR);
    if (ret != 0) {
        return -1;
    }
    k_msleep(BH1750_MEAS_DELAY_MS);
    uint8_t buf[2];
    ret = i2c_read(dev, buf, 2, BH1750_ADDR);
    if (ret != 0) {
        return -1;
    }
    *raw = (int32_t)(((uint16_t)buf[0] << 8) | buf[1]);
    return 0;
}