#include "dps310.h"
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include "arch.h"

#include "nuttx.h"
#define DPS310_REG_PSR_B2 0x00
#define DPS310_REG_TMP_B2 0x03
#define DPS310_REG_COEF   0x10
#define DPS310_REG_MEAS_CFG 0x08
#define DPS310_REG_INT_STS  0x0A
#define DPS310_REG_FIFO_STS 0x0B
#define DPS310_REG_RESET    0x0C
#define DPS310_REG_ID       0x0D

static int dps310_i2c_write_then_read(struct dps310_dev *dev, uint8_t reg, uint8_t *buf, int len)
{
    struct i2c_config_s config;
    config.frequency = 100000;
    config.address = dev->addr;
    config.addrlen = 7;
    int ret = I2C_WRITEREAD(dev->bus, &config, &reg, 1, buf, len);
    if (ret < 0) {
        return -EIO;
    }
    return 0;
}

static int dps310_i2c_write(struct dps310_dev *dev, uint8_t reg, uint8_t data)
{
    struct i2c_config_s config;
    config.frequency = 100000;
    config.address = dev->addr;
    config.addrlen = 7;
    uint8_t buf[2] = {reg, data};
    int ret = I2C_WRITE(dev->bus, &config, buf, 2);
    if (ret < 0) {
        return -EIO;
    }
    return 0;
}

int dps310_init(struct dps310_dev *dev, struct i2c_master_s *bus)
{
    dev->bus = bus;
    dev->addr = DPS310_I2C_ADDR;

    up_mdelay(12);
    up_mdelay(40);

    uint8_t id;
    int ret = dps310_i2c_write_then_read(dev, DPS310_REG_ID, &id, 1);
    if (ret < 0) return ret;

    ret = dps310_i2c_write(dev, DPS310_REG_RESET, 0x09);
    if (ret < 0) return ret;

    uint8_t coef[18];
    ret = dps310_i2c_write_then_read(dev, DPS310_REG_COEF, coef, 18);
    if (ret < 0) return ret;

    return 0;
}

int dps310_read_pressure(struct dps310_dev *dev, int32_t *pressure_raw)
{
    uint8_t buf[3];
    int ret = dps310_i2c_write_then_read(dev, DPS310_REG_PSR_B2, buf, 3);
    if (ret < 0) return ret;
    int32_t raw = (int32_t)((buf[0] << 16) | (buf[1] << 8) | buf[2]);
    if (raw & 0x800000) {
        raw |= 0xFF000000;
    }
    *pressure_raw = raw;
    return 0;
}

int dps310_read_temp(struct dps310_dev *dev, int32_t *temp_raw)
{
    uint8_t buf[3];
    int ret = dps310_i2c_write_then_read(dev, DPS310_REG_TMP_B2, buf, 3);
    if (ret < 0) return ret;
    int32_t raw = (int32_t)((buf[0] << 16) | (buf[1] << 8) | buf[2]);
    if (raw & 0x800000) {
        raw |= 0xFF000000;
    }
    *temp_raw = raw;
    return 0;
}
