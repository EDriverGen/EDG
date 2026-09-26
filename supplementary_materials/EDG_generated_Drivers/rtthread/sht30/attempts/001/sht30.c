#include "sht30.h"
#include "dev_i2c.h"
#include "rtthread.h"
#include <stdint.h>

#define SHT30_I2C_ADDR 0x44
#define SHT30_CMD_MEAS_HIGH_NO_CLOCK_STRETCH 0x2400
#define SHT30_CMD_MEAS_HIGH_CLOCK_STRETCH 0x2C06
#define SHT30_CMD_FETCH_DATA 0xE000
#define SHT30_CMD_SOFT_RESET 0x30A2
#define SHT30_CMD_READ_STATUS 0xF32D

static int sht30_write_command(struct sht30_device *dev, uint16_t cmd)
{
    struct rt_i2c_msg msg;
    uint8_t buf[2];
    buf[0] = (cmd >> 8) & 0xFF;
    buf[1] = cmd & 0xFF;
    msg.addr = dev->i2c_addr;
    msg.flags = RT_I2C_WR;
    msg.len = 2;
    msg.buf = buf;
    if (rt_i2c_transfer(dev->bus, &msg, 1) != 1)
        return -1;
    return 0;
}

static int sht30_read_data(struct sht30_device *dev, uint8_t *buf, uint16_t len)
{
    struct rt_i2c_msg msg;
    msg.addr = dev->i2c_addr;
    msg.flags = RT_I2C_RD;
    msg.len = len;
    msg.buf = buf;
    if (rt_i2c_transfer(dev->bus, &msg, 1) != 1)
        return -1;
    return 0;
}

static int sht30_write_then_read(struct sht30_device *dev, uint16_t cmd, uint8_t *buf, uint16_t len)
{
    struct rt_i2c_msg msgs[2];
    uint8_t cmd_buf[2];
    cmd_buf[0] = (cmd >> 8) & 0xFF;
    cmd_buf[1] = cmd & 0xFF;
    msgs[0].addr = dev->i2c_addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].len = 2;
    msgs[0].buf = cmd_buf;
    msgs[1].addr = dev->i2c_addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].len = len;
    msgs[1].buf = buf;
    if (rt_i2c_transfer(dev->bus, msgs, 2) != 2)
        return -1;
    return 0;
}

int sht30_init(struct sht30_device *dev, struct rt_i2c_bus_device *bus)
{
    dev->bus = bus;
    dev->i2c_addr = SHT30_I2C_ADDR;
    return 0;
}

int sht30_read_temperature(struct sht30_device *dev, int32_t *temperature_milliC)
{
    uint8_t buf[6];
    int32_t st;
    int32_t temp;
    if (sht30_write_then_read(dev, SHT30_CMD_MEAS_HIGH_NO_CLOCK_STRETCH, buf, 6) != 0)
        return -1;
    rt_thread_mdelay(15);
    st = ((int32_t)buf[0] << 8) | buf[1];
    temp = ((st * 175000) / 65535) - 45000;
    *temperature_milliC = temp;
    return 0;
}

int sht30_read_humidity(struct sht30_device *dev, int32_t *humidity_milliPercent)
{
    uint8_t buf[6];
    int32_t srh;
    int32_t hum;
    if (sht30_write_then_read(dev, SHT30_CMD_MEAS_HIGH_NO_CLOCK_STRETCH, buf, 6) != 0)
        return -1;
    rt_thread_mdelay(15);
    srh = ((int32_t)buf[3] << 8) | buf[4];
    hum = (srh * 100000) / 65535;
    *humidity_milliPercent = hum;
    return 0;
}