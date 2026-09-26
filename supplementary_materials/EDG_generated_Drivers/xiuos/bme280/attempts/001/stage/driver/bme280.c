#include "bme280.h"
#include "transform.h"
#include "bus.h"
#include "dev_i2c.h"
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "bus_i2c.h"
#include "bus_pin.h"
#define BME280_I2C_ADDR 0x76
#define BME280_CHIP_ID_REG 0xD0
#define BME280_RESET_REG 0xE0
#define BME280_RESET_CMD 0xB6
#define BME280_CONFIG_REG 0xF5
#define BME280_CTRL_HUM_REG 0xF2
#define BME280_CTRL_MEAS_REG 0xF4
#define BME280_STATUS_REG 0xF3
#define BME280_PRESS_MSB_REG 0xF7
#define BME280_TEMP_MSB_REG 0xFA
#define BME280_HUM_MSB_REG 0xFD
#define BME280_CALIB_REG 0x88
#define BME280_CALIB_HUM_REG 0xE1

static int i2c_write_then_read(struct bme280_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    int ret;
    struct PrivIoctlCfg ioctl_cfg;
    uint16_t addr = dev->i2c_addr;
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
    int ret;
    struct PrivIoctlCfg ioctl_cfg;
    uint16_t addr = dev->i2c_addr;
    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &addr;
    ret = PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg);
    if (ret < 0) return -EIO;
    uint8_t buf[2] = {reg, data};
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
    struct PrivIoctlCfg ioctl_cfg;
    uint16_t addr = dev->i2c_addr;
    ioctl_cfg.ioctl_driver_type = I2C_TYPE;
    ioctl_cfg.args = &addr;
    ret = PrivIoctl(dev->fd, OPE_INT, &ioctl_cfg);
    if (ret < 0) { PrivClose(dev->fd); return -EIO; }
    ret = i2c_write_then_read(dev, BME280_CHIP_ID_REG, &chip_id, 1);
    if (ret < 0) { PrivClose(dev->fd); return ret; }
    if (chip_id != 0x60) { PrivClose(dev->fd); return -EIO; }
    ret = i2c_write(dev, BME280_RESET_REG, BME280_RESET_CMD);
    if (ret < 0) { PrivClose(dev->fd); return ret; }
    PrivTaskDelay(2);
    ret = i2c_write(dev, BME280_CONFIG_REG, 0x00);
    if (ret < 0) { PrivClose(dev->fd); return ret; }
    PrivTaskDelay(2);
    return 0;
}

int bme280_read_all(struct bme280_dev *dev, int32_t *temp_raw, int32_t *pressure_raw, int32_t *humidity_raw)
{
    int ret;
    uint8_t buf[8];
    ret = i2c_write_then_read(dev, BME280_PRESS_MSB_REG, buf, 8);
    if (ret < 0) return ret;
    uint32_t temp = ((uint32_t)buf[3] << 12) | ((uint32_t)buf[4] << 4) | ((uint32_t)buf[5] >> 4);
    uint32_t press = ((uint32_t)buf[0] << 12) | ((uint32_t)buf[1] << 4) | ((uint32_t)buf[2] >> 4);
    uint16_t hum = ((uint16_t)buf[6] << 8) | buf[7];
    *temp_raw = (int32_t)temp;
    *pressure_raw = (int32_t)press;
    *humidity_raw = (int32_t)hum;
    return 0;
}
