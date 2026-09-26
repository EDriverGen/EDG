#include "adxl345.h"
#include <rtthread.h>
#include <rtdevice.h>
#include <stdint.h>

#define ADXL345_DEVID          0x00
#define ADXL345_POWER_CTL      0x2D
#define ADXL345_DATA_FORMAT    0x31
#define ADXL345_BW_RATE        0x2C
#define ADXL345_DATAX0         0x32

#define ADXL345_READ_BIT       0x80
#define ADXL345_MB_BIT         0x40

static int adxl345_write_reg(struct adxl345_device *dev, uint8_t reg, uint8_t val)
{
    uint8_t tx[2] = { reg, val };
    rt_size_t ret = rt_spi_send_then_recv(dev->spi, tx, 2, RT_NULL, 0);
    return (ret == RT_EOK) ? 0 : -1;
}

static int adxl345_read_regs(struct adxl345_device *dev, uint8_t reg, uint8_t *buf, uint8_t len)
{
    uint8_t cmd = reg | ADXL345_READ_BIT;
    if (len > 1) {
        cmd |= ADXL345_MB_BIT;
    }
    rt_size_t ret = rt_spi_send_then_recv(dev->spi, &cmd, 1, buf, len);
    return (ret == RT_EOK) ? 0 : -1;
}

int adxl345_init(struct adxl345_device *dev, struct rt_spi_device *spi)
{
    dev->spi = spi;
    uint8_t devid;
    if (adxl345_read_regs(dev, ADXL345_DEVID, &devid, 1) != 0) {
        return -1;
    }
    if (devid != 0xE5) {
        return -1;
    }
    if (adxl345_write_reg(dev, ADXL345_POWER_CTL, 0x00) != 0) return -1;
    if (adxl345_write_reg(dev, ADXL345_DATA_FORMAT, 0x0B) != 0) return -1;
    if (adxl345_write_reg(dev, ADXL345_BW_RATE, 0x0A) != 0) return -1;
    if (adxl345_write_reg(dev, ADXL345_POWER_CTL, 0x08) != 0) return -1;
    rt_thread_mdelay(12);
    return 0;
}

static int adxl345_read_raw(struct adxl345_device *dev, int32_t *x, int32_t *y, int32_t *z)
{
    uint8_t buf[6];
    if (adxl345_read_regs(dev, ADXL345_DATAX0, buf, 6) != 0) {
        return -1;
    }
    int16_t raw_x = (int16_t)(buf[0] | (buf[1] << 8));
    int16_t raw_y = (int16_t)(buf[2] | (buf[3] << 8));
    int16_t raw_z = (int16_t)(buf[4] | (buf[5] << 8));
    *x = raw_x;
    *y = raw_y;
    *z = raw_z;
    return 0;
}

int adxl345_read_accel_x(struct adxl345_device *dev, int32_t *ax)
{
    int32_t x, y, z;
    int ret = adxl345_read_raw(dev, &x, &y, &z);
    if (ret != 0) return ret;
    *ax = x;
    return 0;
}

int adxl345_read_accel_y(struct adxl345_device *dev, int32_t *ay)
{
    int32_t x, y, z;
    int ret = adxl345_read_raw(dev, &x, &y, &z);
    if (ret != 0) return ret;
    *ay = y;
    return 0;
}

int adxl345_read_accel_z(struct adxl345_device *dev, int32_t *az)
{
    int32_t x, y, z;
    int ret = adxl345_read_raw(dev, &x, &y, &z);
    if (ret != 0) return ret;
    *az = z;
    return 0;
}