#include "bme280.h"
#include "hal.h"
#include "hal_i2c.h"
#include <string.h>

#define BME280_I2C_ADDR 0x76
#define BME280_ID_REG 0xD0
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
#define BME280_TIMEOUT MS2ST(100)

static int bme280_write_reg(struct bme280_dev *dev, uint8_t reg, uint8_t data)
{
    uint8_t txbuf[2] = {reg, data};
    i2cAcquireBus((I2CDriver *)dev->bus_handle);
    msg_t ret = i2cMasterTransmitTimeout((I2CDriver *)dev->bus_handle, dev->i2c_addr, txbuf, 2, NULL, 0, BME280_TIMEOUT);
    i2cReleaseBus((I2CDriver *)dev->bus_handle);
    return (ret == MSG_OK) ? 0 : -1;
}

static int bme280_read_regs(struct bme280_dev *dev, uint8_t reg, uint8_t *buf, size_t len)
{
    uint8_t txbuf[1] = {reg};
    i2cAcquireBus((I2CDriver *)dev->bus_handle);
    msg_t ret = i2cMasterTransmitTimeout((I2CDriver *)dev->bus_handle, dev->i2c_addr, txbuf, 1, buf, len, BME280_TIMEOUT);
    i2cReleaseBus((I2CDriver *)dev->bus_handle);
    return (ret == MSG_OK) ? 0 : -1;
}

int bme280_init(struct bme280_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = BME280_I2C_ADDR;

    uint8_t chip_id = 0;
    if (bme280_read_regs(dev, BME280_ID_REG, &chip_id, 1) != 0)
        return -1;
    if (chip_id != 0x60)
        return -1;

    if (bme280_write_reg(dev, BME280_RESET_REG, BME280_RESET_CMD) != 0)
        return -1;

    chThdSleepMilliseconds(2);

    if (bme280_write_reg(dev, BME280_CONFIG_REG, 0x00) != 0)
        return -1;

    return 0;
}

int bme280_read_all(struct bme280_dev *dev, int32_t *temp_raw, int32_t *pressure_raw, int32_t *humidity_raw)
{
    uint8_t data[8];
    if (bme280_read_regs(dev, BME280_PRESS_MSB_REG, data, 8) != 0)
        return -1;

    uint32_t press = ((uint32_t)data[0] << 12) | ((uint32_t)data[1] << 4) | ((uint32_t)data[2] >> 4);
    uint32_t temp = ((uint32_t)data[3] << 12) | ((uint32_t)data[4] << 4) | ((uint32_t)data[5] >> 4);
    uint16_t hum = ((uint16_t)data[6] << 8) | data[7];

    *temp_raw = (int32_t)temp;
    *pressure_raw = (int32_t)press;
    *humidity_raw = (int32_t)hum;

    return 0;
}