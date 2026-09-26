#include "bme280.h"
#include "transform.h"
#include "bus.h"
#include "bus_i2c.h"
#include "dev_i2c.h"
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "bus_pin.h"
#define BME280_I2C_ADDR 0x76
#define BME280_REG_ID 0xD0
#define BME280_REG_RESET 0xE0
#define BME280_REG_CONFIG 0xF5
#define BME280_REG_CTRL_HUM 0xF2
#define BME280_REG_CTRL_MEAS 0xF4
#define BME280_REG_STATUS 0xF3
#define BME280_REG_PRESS_MSB 0xF7
#define BME280_REG_TEMP_MSB 0xFA
#define BME280_REG_HUM_MSB 0xFD
#define BME280_RESET_CMD 0xB6

static int bme280_write_reg(struct bme280_dev *dev, uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = {reg, value};
    int ret = PrivWrite(dev->fd, buf, 2);
    if (ret < 0) return -EIO;
    return 0;
}

static int bme280_read_regs(struct bme280_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    int ret = PrivWrite(dev->fd, &reg, 1);
    if (ret < 0) return -EIO;
    ret = PrivRead(dev->fd, buf, len);
    if (ret < 0) return -EIO;
    return 0;
}

int bme280_init(struct bme280_dev *dev, struct I2cBus *bus_handle)
{
    int ret;
    struct PrivIoctlCfg ioctl_cfg;
    uint16_t i2c_addr = BME280_I2C_ADDR;

    dev->bus = bus_handle;
    dev->i2c_addr = BME280_I2C_ADDR;

    dev->fd = PrivOpen("/dev/i2c1", 0);
    if (dev->fd < 0) return -EIO;

    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &i2c_addr;
    ret = PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg);
    if (ret < 0) {
        PrivClose(dev->fd);
        return -EIO;
    }

    /* Read chip ID */
    uint8_t id;
    ret = bme280_read_regs(dev, BME280_REG_ID, &id, 1);
    if (ret < 0 || id != 0x60) {
        PrivClose(dev->fd);
        return -EIO;
    }

    /* Soft reset */
    ret = bme280_write_reg(dev, BME280_REG_RESET, BME280_RESET_CMD);
    if (ret < 0) {
        PrivClose(dev->fd);
        return -EIO;
    }
    PrivTaskDelay(2);

    /* Write config register (filter off, standby 0.5ms) */
    ret = bme280_write_reg(dev, BME280_REG_CONFIG, 0x00);
    if (ret < 0) {
        PrivClose(dev->fd);
        return -EIO;
    }

    /* Set humidity oversampling to 1x */
    ret = bme280_write_reg(dev, BME280_REG_CTRL_HUM, 0x01);
    if (ret < 0) {
        PrivClose(dev->fd);
        return -EIO;
    }

    /* Set pressure/temp oversampling to 1x, normal mode */
    ret = bme280_write_reg(dev, BME280_REG_CTRL_MEAS, 0x27);
    if (ret < 0) {
        PrivClose(dev->fd);
        return -EIO;
    }

    PrivTaskDelay(10);

    return 0;
}

int bme280_read_all(struct bme280_dev *dev, int32_t *temp_raw, int32_t *pressure_raw, int32_t *humidity_raw)
{
    int ret;
    uint8_t buf[8];

    /* Burst read from 0xF7, 8 bytes */
    ret = bme280_read_regs(dev, BME280_REG_PRESS_MSB, buf, 8);
    if (ret < 0) return ret;

    /* Pressure: 20-bit, buf[0..2] */
    uint32_t press = ((uint32_t)buf[0] << 12) | ((uint32_t)buf[1] << 4) | ((uint32_t)buf[2] >> 4);
    /* Temperature: 20-bit, buf[3..5] */
    uint32_t temp = ((uint32_t)buf[3] << 12) | ((uint32_t)buf[4] << 4) | ((uint32_t)buf[5] >> 4);
    /* Humidity: 16-bit, buf[6..7] */
    uint16_t hum = ((uint16_t)buf[6] << 8) | buf[7];

    *temp_raw = (int32_t)temp;
    *pressure_raw = (int32_t)press;
    *humidity_raw = (int32_t)hum;

    return 0;
}
