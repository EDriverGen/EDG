#include "sht30.h"
#include "dev_i2c.h"
#include "rtthread.h"

#define SHT30_I2C_ADDR 0x44
#define SHT30_CMD_MEASURE_HIGH_NOCLK 0x2400
#define SHT30_CMD_MEASURE_HIGH_CLK 0x2C06
#define SHT30_CMD_FETCH_DATA 0xE000
#define SHT30_CMD_SOFT_RESET 0x30A2
#define SHT30_CMD_BREAK 0x3093
#define SHT30_CMD_READ_STATUS 0xF32D
#define SHT30_CMD_GENERAL_CALL_RESET 0x0006

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

int sht30_init(struct sht30_device *dev, struct rt_i2c_bus_device *bus)
{
    dev->bus = bus;
    dev->i2c_addr = SHT30_I2C_ADDR;
    return 0;
}

int sht30_read_temperature(struct sht30_device *dev, int32_t *temperature_milliC)
{
    uint8_t buf[6];
    int ret;
    ret = sht30_write_command(dev, SHT30_CMD_MEASURE_HIGH_NOCLK);
    if (ret != 0)
        return -1;
    rt_thread_mdelay(15);
    ret = sht30_read_data(dev, buf, 6);
    if (ret != 0)
        return -1;
    uint16_t st = ((uint16_t)buf[0] << 8) | buf[1];
    int64_t temp = ((int64_t)st * 175000) / 65535 - 45000;
    *temperature_milliC = (int32_t)temp;
    return 0;
}

int sht30_read_humidity(struct sht30_device *dev, int32_t *humidity_milliPercent)
{
    uint8_t buf[6];
    int ret;
    ret = sht30_write_command(dev, SHT30_CMD_MEASURE_HIGH_NOCLK);
    if (ret != 0)
        return -1;
    rt_thread_mdelay(15);
    ret = sht30_read_data(dev, buf, 6);
    if (ret != 0)
        return -1;
    uint16_t srh = ((uint16_t)buf[3] << 8) | buf[4];
    int64_t hum = ((int64_t)srh * 100000) / 65535;
    *humidity_milliPercent = (int32_t)hum;
    return 0;
}
