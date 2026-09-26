#include "sht30.h"
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include <arch.h>

#define SHT30_I2C_ADDR 0x44

#define CMD_SOFT_RESET 0x30A2
#define CMD_GENERAL_CALL_RESET 0x0006
#define CMD_SINGLE_SHOT_HIGH_NO_CLOCK_STRETCH 0x2400

static int sht30_write_command(struct sht30_dev_s *dev, uint16_t cmd)
{
    struct i2c_msg_s msg;
    uint8_t buf[2];
    buf[0] = (cmd >> 8) & 0xFF;
    buf[1] = cmd & 0xFF;
    msg.frequency = 100000;
    msg.addr = SHT30_I2C_ADDR;
    msg.flags = 0;
    msg.buffer = buf;
    msg.length = 2;
    return I2C_TRANSFER(dev->bus, &msg, 1);
}

static int sht30_read_data(struct sht30_dev_s *dev, uint8_t *buf, int len)
{
    struct i2c_msg_s msg;
    msg.frequency = 100000;
    msg.addr = SHT30_I2C_ADDR;
    msg.flags = I2C_M_READ;
    msg.buffer = buf;
    msg.length = len;
    return I2C_TRANSFER(dev->bus, &msg, 1);
}

int sht30_init(struct sht30_dev_s *dev, struct i2c_master_s *bus)
{
    int ret;
    dev->bus = bus;
    dev->addr = SHT30_I2C_ADDR;

    /* Soft reset */
    ret = sht30_write_command(dev, CMD_SOFT_RESET);
    if (ret < 0) return ret;
    up_mdelay(2);

    /* General call reset */
    ret = sht30_write_command(dev, CMD_GENERAL_CALL_RESET);
    if (ret < 0) return ret;
    up_mdelay(2);

    return 0;
}

int sht30_read_measurement(struct sht30_dev_s *dev, int32_t *temp_milliC, int32_t *hum_milliPct)
{
    int ret;
    uint8_t buf[6];
    uint16_t st, srh;

    /* Send single shot measurement command */
    ret = sht30_write_command(dev, CMD_SINGLE_SHOT_HIGH_NO_CLOCK_STRETCH);
    if (ret < 0) return ret;

    /* Wait for measurement */
    up_mdelay(15);

    /* Read 6 bytes */
    ret = sht30_read_data(dev, buf, 6);
    if (ret < 0) return ret;

    /* CRC check not implemented for simplicity, assume valid */
    st = ((uint16_t)buf[0] << 8) | buf[1];
    srh = ((uint16_t)buf[3] << 8) | buf[4];

    /* Convert using integer approximation */
    *temp_milliC = ((int32_t)(((int64_t)st * 175000) / 65535) - 45000);
    *hum_milliPct = (int32_t)(((int64_t)srh * 100000) / 65535);

    return 0;
}