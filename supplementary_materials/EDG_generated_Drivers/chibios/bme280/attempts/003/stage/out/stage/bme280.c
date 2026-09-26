#include "bme280.h"
#include "hal.h"
#include "hal_i2c.h"
#include <string.h>

#define BME280_I2C_ADDR 0x76
#define BME280_REG_ID 0xD0
#define BME280_REG_RESET 0xE0
#define BME280_REG_CTRL_HUM 0xF2
#define BME280_REG_STATUS 0xF3
#define BME280_REG_CTRL_MEAS 0xF4
#define BME280_REG_CONFIG 0xF5
#define BME280_REG_PRESS_MSB 0xF7
#define BME280_REG_TEMP_MSB 0xFA
#define BME280_REG_HUM_MSB 0xFD
#define BME280_RESET_WORD 0xB6
#define BME280_CHIP_ID 0x60
#define TIMEOUT_MS 100

static int i2c_write_then_read(struct bme280_dev *dev, uint8_t reg, uint8_t *buf, size_t len)
{
    i2cAcquireBus((I2CDriver *)dev->bus_handle);
    msg_t ret = i2cMasterTransmitTimeout((I2CDriver *)dev->bus_handle, dev->i2c_addr, &reg, 1, buf, len, MS2ST(TIMEOUT_MS));
    i2cReleaseBus((I2CDriver *)dev->bus_handle);
    return (ret == MSG_OK) ? 0 : -1;
}

static int i2c_write(struct bme280_dev *dev, uint8_t reg, uint8_t data)
{
    uint8_t txbuf[2] = {reg, data};
    i2cAcquireBus((I2CDriver *)dev->bus_handle);
    msg_t ret = i2cMasterTransmitTimeout((I2CDriver *)dev->bus_handle, dev->i2c_addr, txbuf, 2, NULL, 0, MS2ST(TIMEOUT_MS));
    i2cReleaseBus((I2CDriver *)dev->bus_handle);
    return (ret == MSG_OK) ? 0 : -1;
}

int bme280_init(struct bme280_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = BME280_I2C_ADDR;

    uint8_t chip_id = 0;
    if (i2c_write_then_read(dev, BME280_REG_ID, &chip_id, 1) != 0)
        return -1;
    if (chip_id != BME280_CHIP_ID)
        return -1;

    if (i2c_write(dev, BME280_REG_RESET, BME280_RESET_WORD) != 0)
        return -1;

    chThdSleepMilliseconds(2);

    if (i2c_write(dev, BME280_REG_CONFIG, 0x00) != 0)
        return -1;

    return 0;
}

int bme280_read_all(struct bme280_dev *dev, int32_t *temp_raw, int32_t *pressure_raw, int32_t *humidity_raw)
{
    uint8_t buf[8];
    if (i2c_write_then_read(dev, BME280_REG_PRESS_MSB, buf, 8) != 0)
        return -1;

    uint32_t press = ((uint32_t)buf[0] << 12) | ((uint32_t)buf[1] << 4) | ((uint32_t)buf[2] >> 4);
    uint32_t temp = ((uint32_t)buf[3] << 12) | ((uint32_t)buf[4] << 4) | ((uint32_t)buf[5] >> 4);
    uint16_t hum = ((uint16_t)buf[6] << 8) | buf[7];

    *temp_raw = (int32_t)temp;
    *pressure_raw = (int32_t)press;
    *humidity_raw = (int32_t)hum;

    return 0;
}