#include "bme280.h"
#include "i2c_msg.h"
#include "hdf_base.h"
#include "osal_time.h"
#include <stdint.h>

#include "openharmony_liteosm.h"
#define BME280_I2C_ADDR 0x76
#define BME280_CHIP_ID_REG 0xD0
#define BME280_RESET_REG 0xE0
#define BME280_RESET_CMD 0xB6
#define BME280_CONFIG_REG 0xF5
#define BME280_STATUS_REG 0xF3
#define BME280_CTRL_MEAS_REG 0xF4
#define BME280_CTRL_HUM_REG 0xF2
#define BME280_PRESS_MSB_REG 0xF7
#define BME280_TEMP_MSB_REG 0xFA
#define BME280_HUM_MSB_REG 0xFD
#define BME280_CALIB_REG 0x88
#define BME280_CALIB_HUM_REG 0xE1
#define BME280_CALIB_LEN 26
#define BME280_CALIB_HUM_LEN 7
#define BME280_DATA_LEN 8
#define BME280_CHIP_ID_EXPECTED 0x60

static int32_t bme280_write_then_read(struct bme280_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct I2cMsg msgs[2];
    int32_t ret;

    msgs[0].addr = dev->i2c_addr;
    msgs[0].buf = &reg;
    msgs[0].len = 1;
    msgs[0].flags = 0;

    msgs[1].addr = dev->i2c_addr;
    msgs[1].buf = buf;
    msgs[1].len = len;
    msgs[1].flags = I2C_FLAG_READ;

    ret = I2cTransfer(dev->bus_handle, msgs, 2);
    if (ret != 2) {
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t bme280_write_reg(struct bme280_dev *dev, uint8_t reg, uint8_t value)
{
    struct I2cMsg msgs[1];
    uint8_t buf[2];
    int32_t ret;

    buf[0] = reg;
    buf[1] = value;

    msgs[0].addr = dev->i2c_addr;
    msgs[0].buf = buf;
    msgs[0].len = 2;
    msgs[0].flags = 0;

    ret = I2cTransfer(dev->bus_handle, msgs, 1);
    if (ret != 1) {
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t bme280_init(struct bme280_dev *dev, DevHandle bus_handle)
{
    uint8_t chip_id = 0;
    int32_t ret;

    dev->bus_handle = bus_handle;
    dev->i2c_addr = BME280_I2C_ADDR;

    /* Read chip ID */
    ret = bme280_write_then_read(dev, BME280_CHIP_ID_REG, &chip_id, 1);
    if (ret != HDF_SUCCESS) {
        return HDF_FAILURE;
    }
    if (chip_id != BME280_CHIP_ID_EXPECTED) {
        return HDF_FAILURE;
    }

    /* Reset device */
    ret = bme280_write_reg(dev, BME280_RESET_REG, BME280_RESET_CMD);
    if (ret != HDF_SUCCESS) {
        return HDF_FAILURE;
    }
    OsalMSleep(2);

    /* Write config register (filter off, standby 0.5ms) */
    ret = bme280_write_reg(dev, BME280_CONFIG_REG, 0x00);
    if (ret != HDF_SUCCESS) {
        return HDF_FAILURE;
    }

    /* Set ctrl_hum to oversampling x1 */
    ret = bme280_write_reg(dev, BME280_CTRL_HUM_REG, 0x01);
    if (ret != HDF_SUCCESS) {
        return HDF_FAILURE;
    }

    /* Set ctrl_meas: oversampling temp x1, pressure x1, normal mode */
    ret = bme280_write_reg(dev, BME280_CTRL_MEAS_REG, 0x27);
    if (ret != HDF_SUCCESS) {
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t bme280_read_all(struct bme280_dev *dev, int32_t *temp_raw, int32_t *pressure_raw, int32_t *humidity_raw)
{
    uint8_t data[BME280_DATA_LEN];
    uint32_t raw_temp, raw_press;
    uint16_t raw_hum;
    int32_t ret;

    /* Burst read from 0xF7, 8 bytes */
    ret = bme280_write_then_read(dev, BME280_PRESS_MSB_REG, data, BME280_DATA_LEN);
    if (ret != HDF_SUCCESS) {
        return HDF_FAILURE;
    }

    /* Pressure: bytes 0-2, 20-bit, right-shift 4 */
    raw_press = ((uint32_t)data[0] << 16) | ((uint32_t)data[1] << 8) | (uint32_t)data[2];
    raw_press >>= 4;

    /* Temperature: bytes 3-5, 20-bit, right-shift 4 */
    raw_temp = ((uint32_t)data[3] << 16) | ((uint32_t)data[4] << 8) | (uint32_t)data[5];
    raw_temp >>= 4;

    /* Humidity: bytes 6-7, 16-bit */
    raw_hum = ((uint16_t)data[6] << 8) | (uint16_t)data[7];

    *temp_raw = (int32_t)raw_temp;
    *pressure_raw = (int32_t)raw_press;
    *humidity_raw = (int32_t)raw_hum;

    return HDF_SUCCESS;
}
