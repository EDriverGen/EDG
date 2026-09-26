#include "sht30.h"
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include "arch.h"

#define SHT30_I2C_ADDR 0x44
#define SHT30_CMD_SOFT_RESET 0x30A2
#define SHT30_CMD_GENERAL_CALL_RESET 0x0006
#define SHT30_CMD_SINGLE_SHOT_HIGH_NO_CLOCK_STRETCH 0x2400
#define SHT30_READ_LEN 6

static int sht30_write_command(struct sht30_dev_s *dev, uint16_t cmd)
{
    struct i2c_config_s config;
    config.frequency = 100000;
    config.address = dev->addr;
    config.addrlen = 7;
    uint8_t buf[2];
    buf[0] = (cmd >> 8) & 0xFF;
    buf[1] = cmd & 0xFF;
    int ret = I2C_TRANSFER(dev->bus, &config, buf, 2);
    if (ret < 0) {
        return ret;
    }
    return OK;
}

static int sht30_write_command_general_call(uint16_t cmd)
{
    struct i2c_config_s config;
    config.frequency = 100000;
    config.address = 0x00;
    config.addrlen = 7;
    uint8_t buf[2];
    buf[0] = (cmd >> 8) & 0xFF;
    buf[1] = cmd & 0xFF;
    int ret = I2C_TRANSFER(dev->bus, &config, buf, 2);
    if (ret < 0) {
        return ret;
    }
    return OK;
}

static int sht30_read_bytes(struct sht30_dev_s *dev, uint8_t *buf, int len)
{
    struct i2c_config_s config;
    config.frequency = 100000;
    config.address = dev->addr;
    config.addrlen = 7;
    int ret = I2C_TRANSFER(dev->bus, &config, buf, len);
    if (ret < 0) {
        return ret;
    }
    return OK;
}

int sht30_init(struct sht30_dev_s *dev, struct i2c_master_s *bus)
{
    if (!dev || !bus) {
        return -EINVAL;
    }
    dev->bus = bus;
    dev->addr = SHT30_I2C_ADDR;

    int ret;
    ret = sht30_write_command(dev, SHT30_CMD_SOFT_RESET);
    if (ret < 0) {
        return ret;
    }
    up_mdelay(2);

    ret = sht30_write_command_general_call(SHT30_CMD_GENERAL_CALL_RESET);
    if (ret < 0) {
        return ret;
    }
    up_mdelay(2);

    return 0;
}

int sht30_read_measurement(struct sht30_dev_s *dev, int32_t *temp_milliC, int32_t *hum_milliPct)
{
    if (!dev || !temp_milliC || !hum_milliPct) {
        return -EINVAL;
    }

    int ret;
    ret = sht30_write_command(dev, SHT30_CMD_SINGLE_SHOT_HIGH_NO_CLOCK_STRETCH);
    if (ret < 0) {
        return ret;
    }

    up_mdelay(15);

    uint8_t buf[SHT30_READ_LEN];
    ret = sht30_read_bytes(dev, buf, SHT30_READ_LEN);
    if (ret < 0) {
        return ret;
    }

    uint16_t st = ((uint16_t)buf[0] << 8) | buf[1];
    uint16_t srh = ((uint16_t)buf[3] << 8) | buf[4];

    int32_t temp = ((int32_t)st * 175000) / 65535 - 45000;
    int32_t hum = ((int32_t)srh * 100000) / 65535;

    *temp_milliC = temp;
    *hum_milliPct = hum;

    return 0;
}