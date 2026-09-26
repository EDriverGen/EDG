#include "bme280.h"
#include "xtimer.h"
#include <errno.h>
#include <string.h>

#include "riot.h"
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
#define BME280_CALIB_LEN 26
#define BME280_CALIB_HUM_LEN 7
#define BME280_DATA_LEN 8

static int read_regs(bme280_t *dev, uint16_t reg, void *data, size_t len) {
    return i2c_read_regs(dev->bus, dev->addr, reg, data, len, 0);
}

static int write_reg(bme280_t *dev, uint16_t reg, uint8_t value) {
    uint8_t buf[2] = { (uint8_t)(reg & 0xFF), value };
    return i2c_write_bytes(dev->bus, dev->addr, buf, 2, 0);
}

int bme280_init(bme280_t *dev, i2c_t bus) {
    dev->bus = bus;
    dev->addr = BME280_I2C_ADDR;

    uint8_t chip_id;
    int ret = read_regs(dev, BME280_CHIP_ID_REG, &chip_id, 1);
    if (ret != 0) return -EIO;
    if (chip_id != 0x60) return -ENODEV;

    ret = write_reg(dev, BME280_RESET_REG, BME280_RESET_CMD);
    if (ret != 0) return -EIO;
    xtimer_msleep(2);

    ret = write_reg(dev, BME280_CONFIG_REG, 0);
    if (ret != 0) return -EIO;

    return 0;
}

int bme280_read_all(bme280_t *dev, int32_t *temp_raw, int32_t *pressure_raw, int32_t *humidity_raw) {
    uint8_t data[BME280_DATA_LEN];
    int ret = read_regs(dev, BME280_PRESS_MSB_REG, data, BME280_DATA_LEN);
    if (ret != 0) return -EIO;

    uint32_t press = ((uint32_t)data[0] << 12) | ((uint32_t)data[1] << 4) | ((uint32_t)data[2] >> 4);
    uint32_t temp = ((uint32_t)data[3] << 12) | ((uint32_t)data[4] << 4) | ((uint32_t)data[5] >> 4);
    uint16_t hum = ((uint16_t)data[6] << 8) | data[7];

    *temp_raw = (int32_t)temp;
    *pressure_raw = (int32_t)press;
    *humidity_raw = (int32_t)hum;

    return 0;
}
