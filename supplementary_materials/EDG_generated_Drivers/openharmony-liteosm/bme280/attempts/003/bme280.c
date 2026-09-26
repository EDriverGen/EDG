#include "bme280.h"
#include "i2c_if.h"
#include "i2c_msg.h"
#include "hdf_base.h"
#include "osal_time.h"
#include <stdint.h>

#include "openharmony_liteosm.h"
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

static int32_t bme280_write_reg(struct bme280_dev *dev, uint8_t reg, uint8_t data)
{
    struct I2cMsg msgs[1];
    uint8_t buf[2] = {reg, data};
    msgs[0].addr = dev->i2c_addr;
    msgs[0].buf = buf;
    msgs[0].len = 2;
    msgs[0].flags = 0;
    int32_t ret = I2cTransfer(dev->bus_handle, msgs, 1);
    if (ret != 1) {
        return -1;
    }
    return 0;
}

static int32_t bme280_read_regs(struct bme280_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct I2cMsg msgs[2];
    msgs[0].addr = dev->i2c_addr;
    msgs[0].buf = &reg;
    msgs[0].len = 1;
    msgs[0].flags = 0;
    msgs[1].addr = dev->i2c_addr;
    msgs[1].buf = buf;
    msgs[1].len = len;
    msgs[1].flags = I2C_FLAG_READ;
    int32_t ret = I2cTransfer(dev->bus_handle, msgs, 2);
    if (ret != 2) {
        return -1;
    }
    return 0;
}

int32_t bme280_init(struct bme280_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = BME280_I2C_ADDR;

    uint8_t chip_id = 0;
    if (bme280_read_regs(dev, BME280_REG_ID, &chip_id, 1) != 0) {
        return -1;
    }
    if (chip_id != 0x60) {
        return -1;
    }

    if (bme280_write_reg(dev, BME280_REG_RESET, 0xB6) != 0) {
        return -1;
    }
    OsalMSleep(2);

    if (bme280_write_reg(dev, BME280_REG_CONFIG, 0x00) != 0) {
        return -1;
    }

    if (bme280_write_reg(dev, BME280_REG_CTRL_HUM, 0x01) != 0) {
        return -1;
    }
    if (bme280_write_reg(dev, BME280_REG_CTRL_MEAS, 0x27) != 0) {
        return -1;
    }

    return 0;
}

int32_t bme280_read_all(struct bme280_dev *dev, int32_t *temp_raw, int32_t *pressure_raw, int32_t *humidity_raw)
{
    uint8_t buf[8];
    if (bme280_read_regs(dev, BME280_REG_PRESS_MSB, buf, 8) != 0) {
        return -1;
    }

    uint32_t press = ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2];
    press >>= 4;
    uint32_t temp = ((uint32_t)buf[3] << 16) | ((uint32_t)buf[4] << 8) | buf[5];
    temp >>= 4;
    uint16_t hum = ((uint16_t)buf[6] << 8) | buf[7];

    *temp_raw = (int32_t)temp;
    *pressure_raw = (int32_t)press;
    *humidity_raw = (int32_t)hum;

    return 0;
}
