#include "bme280.h"
#include "xtimer.h"
#include <errno.h>
#include <stdint.h>
#include <string.h>

#include "riot.h"
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

static int bme280_write_reg(bme280_t *dev, uint8_t reg, uint8_t value) {
    uint8_t buf[2] = {reg, value};
    int ret = i2c_write_bytes(dev->bus, dev->addr, buf, 2, 0);
    if (ret < 0) return -EIO;
    return 0;
}

static int bme280_read_regs(bme280_t *dev, uint8_t reg, uint8_t *buf, size_t len) {
    int ret = i2c_read_regs(dev->bus, dev->addr, reg, buf, len, 0);
    if (ret < 0) return -EIO;
    return 0;
}

int bme280_init(bme280_t *dev, i2c_t bus) {
    dev->bus = bus;
    dev->addr = BME280_I2C_ADDR;

    uint8_t id;
    if (bme280_read_regs(dev, BME280_REG_ID, &id, 1) < 0) return -EIO;
    if (id != 0x60) return -EIO;

    if (bme280_write_reg(dev, BME280_REG_RESET, 0xB6) < 0) return -EIO;
    xtimer_msleep(2);

    if (bme280_write_reg(dev, BME280_REG_CONFIG, 0x00) < 0) return -EIO;
    xtimer_msleep(2);

    return 0;
}

int bme280_read_all(bme280_t *dev, int32_t *temp_raw, int32_t *pressure_raw, int32_t *humidity_raw) {
    uint8_t buf[8];
    if (bme280_read_regs(dev, BME280_REG_PRESS_MSB, buf, 8) < 0) return -EIO;

    uint32_t press = ((uint32_t)buf[0] << 12) | ((uint32_t)buf[1] << 4) | ((uint32_t)buf[2] >> 4);
    uint32_t temp = ((uint32_t)buf[3] << 12) | ((uint32_t)buf[4] << 4) | ((uint32_t)buf[5] >> 4);
    uint16_t hum = ((uint16_t)buf[6] << 8) | buf[7];

    *temp_raw = (int32_t)temp;
    *pressure_raw = (int32_t)press;
    *humidity_raw = (int32_t)hum;

    return 0;
}
