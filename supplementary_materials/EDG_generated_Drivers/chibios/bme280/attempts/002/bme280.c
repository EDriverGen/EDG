#include "bme280.h"
#include "hal.h"
#include "hal_i2c.h"
#include <string.h>

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
#define BME280_CHIP_ID_EXPECTED 0x60
#define I2C_TIMEOUT_MS 100

static int bme280_write_reg(struct bme280_dev *dev, uint8_t reg, uint8_t data)
{
    uint8_t txbuf[2] = {reg, data};
    msg_t ret = i2cMasterTransmitTimeout(((I2CDriver *)dev->bus_handle), BME280_I2C_ADDR, txbuf, 2, NULL, 0, MS2ST(I2C_TIMEOUT_MS));
    return (ret == MSG_OK) ? 0 : -1;
}

static int bme280_read_regs(struct bme280_dev *dev, uint8_t reg, uint8_t *buf, size_t len)
{
    msg_t ret = i2cMasterTransmitTimeout(((I2CDriver *)dev->bus_handle), BME280_I2C_ADDR, &reg, 1, buf, len, MS2ST(I2C_TIMEOUT_MS));
    return (ret == MSG_OK) ? 0 : -1;
}

int bme280_init(struct bme280_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = BME280_I2C_ADDR;

    i2cAcquireBus((I2CDriver *)dev->bus_handle);

    uint8_t chip_id = 0;
    int ret = bme280_read_regs(dev, BME280_CHIP_ID_REG, &chip_id, 1);
    if (ret != 0 || chip_id != BME280_CHIP_ID_EXPECTED) {
        i2cReleaseBus((I2CDriver *)dev->bus_handle);
        return -1;
    }

    ret = bme280_write_reg(dev, BME280_RESET_REG, BME280_RESET_CMD);
    if (ret != 0) {
        i2cReleaseBus((I2CDriver *)dev->bus_handle);
        return -1;
    }

    chThdSleepMilliseconds(2);

    ret = bme280_write_reg(dev, BME280_CONFIG_REG, 0x00);
    if (ret != 0) {
        i2cReleaseBus((I2CDriver *)dev->bus_handle);
        return -1;
    }

    i2cReleaseBus((I2CDriver *)dev->bus_handle);
    return 0;
}

int bme280_read_all(struct bme280_dev *dev, int32_t *temp_raw, int32_t *pressure_raw, int32_t *humidity_raw)
{
    i2cAcquireBus((I2CDriver *)dev->bus_handle);

    uint8_t buf[8];
    int ret = bme280_read_regs(dev, BME280_PRESS_MSB_REG, buf, 8);
    if (ret != 0) {
        i2cReleaseBus((I2CDriver *)dev->bus_handle);
        return -1;
    }

    uint32_t press = ((uint32_t)buf[0] << 12) | ((uint32_t)buf[1] << 4) | ((uint32_t)buf[2] >> 4);
    uint32_t temp = ((uint32_t)buf[3] << 12) | ((uint32_t)buf[4] << 4) | ((uint32_t)buf[5] >> 4);
    uint16_t hum = ((uint16_t)buf[6] << 8) | buf[7];

    *temp_raw = (int32_t)temp;
    *pressure_raw = (int32_t)press;
    *humidity_raw = (int32_t)hum;

    i2cReleaseBus((I2CDriver *)dev->bus_handle);
    return 0;
}