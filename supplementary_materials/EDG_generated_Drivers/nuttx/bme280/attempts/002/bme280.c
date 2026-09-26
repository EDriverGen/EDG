#include "bme280.h"
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <nuttx/i2c/i2c_master.h>
#include "nuttx.h"
#include "arch.h"

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

static int bme280_write_reg(struct bme280_dev *dev, uint8_t reg, uint8_t value)
{
    struct i2c_msg_s msg[2];
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = value;
    msg[0].frequency = 100000;
    msg[0].addr = dev->addr;
    msg[0].flags = 0;
    msg[0].buffer = buf;
    msg[0].length = 2;
    int ret = I2C_TRANSFER(dev->bus, msg, 1);
    if (ret < 0) {
        return -EIO;
    }
    return 0;
}

static int bme280_read_regs(struct bme280_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct i2c_msg_s msg[2];
    msg[0].frequency = 100000;
    msg[0].addr = dev->addr;
    msg[0].flags = 0;
    msg[0].buffer = &reg;
    msg[0].length = 1;
    msg[1].frequency = 100000;
    msg[1].addr = dev->addr;
    msg[1].flags = I2C_M_READ;
    msg[1].buffer = buf;
    msg[1].length = len;
    int ret = I2C_TRANSFER(dev->bus, msg, 2);
    if (ret < 0) {
        return -EIO;
    }
    return 0;
}

int bme280_init(struct bme280_dev *dev, struct i2c_master_s *bus)
{
    int ret;
    dev->bus = bus;
    dev->addr = BME280_I2C_ADDR;

    /* Read chip ID */
    uint8_t chip_id = 0;
    ret = bme280_read_regs(dev, BME280_CHIP_ID_REG, &chip_id, 1);
    if (ret < 0) return ret;
    if (chip_id != 0x60) return -ENODEV;

    /* Soft reset */
    ret = bme280_write_reg(dev, BME280_RESET_REG, BME280_RESET_CMD);
    if (ret < 0) return ret;
    up_mdelay(2);

    /* Write config register (filter off, standby 0.5ms) */
    ret = bme280_write_reg(dev, BME280_CONFIG_REG, 0x00);
    if (ret < 0) return ret;

    /* Write ctrl_hum (oversampling x1) */
    ret = bme280_write_reg(dev, BME280_CTRL_HUM_REG, 0x01);
    if (ret < 0) return ret;

    /* Write ctrl_meas (oversampling x1 for temp and pressure, normal mode) */
    ret = bme280_write_reg(dev, BME280_CTRL_MEAS_REG, 0x27);
    if (ret < 0) return ret;

    up_mdelay(10);
    return 0;
}

int bme280_read_all(struct bme280_dev *dev, int32_t *temp_raw, int32_t *pressure_raw, int32_t *humidity_raw)
{
    int ret;
    uint8_t buf[8];

    /* Burst read from 0xF7, 8 bytes */
    ret = bme280_read_regs(dev, BME280_PRESS_MSB_REG, buf, 8);
    if (ret < 0) return ret;

    /* Pressure: buf[0..2] big-endian 20-bit */
    uint32_t press = ((uint32_t)buf[0] << 12) | ((uint32_t)buf[1] << 4) | ((uint32_t)buf[2] >> 4);
    /* Temperature: buf[3..5] big-endian 20-bit */
    uint32_t temp = ((uint32_t)buf[3] << 12) | ((uint32_t)buf[4] << 4) | ((uint32_t)buf[5] >> 4);
    /* Humidity: buf[6..7] big-endian 16-bit */
    uint32_t hum = ((uint32_t)buf[6] << 8) | (uint32_t)buf[7];

    *temp_raw = (int32_t)temp;
    *pressure_raw = (int32_t)press;
    *humidity_raw = (int32_t)hum;

    return 0;
}