#include "bme280.h"
#include "transform.h"
#include "bus.h"
#include "bus_i2c.h"
#include "dev_i2c.h"
#include "bus_pin.h"
#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define BME280_CHIP_ID 0x60
#define BME280_RESET_WORD 0xB6

static int i2c_write_then_read(struct bme280_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct PrivIoctlCfg ioctl_cfg;
    uint16_t addr = dev->i2c_addr;
    int ret;

    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &addr;
    ret = PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg);
    if (ret < 0) return -EIO;

    ret = PrivWrite(dev->fd, &reg, 1);
    if (ret < 0) return -EIO;

    ret = PrivRead(dev->fd, buf, len);
    if (ret < 0) return -EIO;

    return 0;
}

static int i2c_write(struct bme280_dev *dev, uint8_t reg, uint8_t data)
{
    struct PrivIoctlCfg ioctl_cfg;
    uint16_t addr = dev->i2c_addr;
    int ret;
    uint8_t buf[2] = {reg, data};

    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &addr;
    ret = PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg);
    if (ret < 0) return -EIO;

    ret = PrivWrite(dev->fd, buf, 2);
    if (ret < 0) return -EIO;

    return 0;
}

int bme280_init(struct bme280_dev *dev, struct I2cBus *bus_handle)
{
    int ret;
    uint8_t chip_id;

    dev->bus = bus_handle;
    dev->i2c_addr = BME280_I2C_ADDR;

    dev->fd = PrivOpen("/dev/i2c1", 0);
    if (dev->fd < 0) return -EIO;

    /* Read chip ID */
    ret = i2c_write_then_read(dev, 0xD0, &chip_id, 1);
    if (ret < 0) { PrivClose(dev->fd); return ret; }
    if (chip_id != BME280_CHIP_ID) { PrivClose(dev->fd); return -ENODEV; }

    /* Soft reset */
    ret = i2c_write(dev, 0xE0, BME280_RESET_WORD);
    if (ret < 0) { PrivClose(dev->fd); return ret; }

    PrivTaskDelay(2);

    /* Write config register (filter off, standby 0.5ms) */
    ret = i2c_write(dev, 0xF5, 0x00);
    if (ret < 0) { PrivClose(dev->fd); return ret; }

    /* Set ctrl_hum (oversampling x1) */
    ret = i2c_write(dev, 0xF2, 0x01);
    if (ret < 0) { PrivClose(dev->fd); return ret; }

    /* Set ctrl_meas (oversampling x1 for temp and pressure, normal mode) */
    ret = i2c_write(dev, 0xF4, 0x27);
    if (ret < 0) { PrivClose(dev->fd); return ret; }

    PrivTaskDelay(10);

    return 0;
}

int bme280_read_all(struct bme280_dev *dev, int32_t *temp_raw, int32_t *pressure_raw, int32_t *humidity_raw)
{
    int ret;
    uint8_t buf[8];
    uint32_t raw_temp, raw_press;
    uint16_t raw_hum;

    /* Burst read from 0xF7, 8 bytes */
    ret = i2c_write_then_read(dev, 0xF7, buf, 8);
    if (ret < 0) return ret;

    /* Temperature: 20-bit, big-endian, right-shift 4 */
    raw_temp = ((uint32_t)buf[0] << 12) | ((uint32_t)buf[1] << 4) | ((uint32_t)buf[2] >> 4);
    /* Pressure: 20-bit, big-endian, right-shift 4 */
    raw_press = ((uint32_t)buf[3] << 12) | ((uint32_t)buf[4] << 4) | ((uint32_t)buf[5] >> 4);
    /* Humidity: 16-bit, big-endian */
    raw_hum = ((uint16_t)buf[6] << 8) | buf[7];

    *temp_raw = (int32_t)raw_temp;
    *pressure_raw = (int32_t)raw_press;
    *humidity_raw = (int32_t)raw_hum;

    return 0;
}