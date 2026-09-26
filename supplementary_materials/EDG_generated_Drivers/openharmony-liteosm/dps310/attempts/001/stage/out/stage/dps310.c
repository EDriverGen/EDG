#include "dps310.h"
#include <stdint.h>
#include <string.h>
#include "i2c_if.h"
#include "i2c_msg.h"
#include "hdf_base.h"
#include "osal_time.h"

#include "openharmony_liteosm.h"
#define DPS310_REG_PSR_B2 0x00
#define DPS310_REG_TMP_B2 0x03
#define DPS310_REG_COEF   0x10
#define DPS310_REG_MEAS_CFG 0x08
#define DPS310_REG_RESET  0x0C
#define DPS310_REG_ID     0x0D

static int32_t dps310_write_then_read(struct dps310_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct I2cMsg msgs[2];
    uint8_t reg_buf = reg;

    msgs[0].addr = dev->i2c_addr;
    msgs[0].buf = &reg_buf;
    msgs[0].len = 1;
    msgs[0].flags = 0;

    msgs[1].addr = dev->i2c_addr;
    msgs[1].buf = buf;
    msgs[1].len = len;
    msgs[1].flags = I2C_FLAG_READ;

    int32_t ret = I2cTransfer(dev->bus_handle, msgs, 2);
    if (ret != 2) {
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

static int32_t dps310_write(struct dps310_dev *dev, uint8_t reg, uint8_t value)
{
    struct I2cMsg msg;
    uint8_t data[2] = {reg, value};

    msg.addr = dev->i2c_addr;
    msg.buf = data;
    msg.len = 2;
    msg.flags = 0;

    int32_t ret = I2cTransfer(dev->bus_handle, &msg, 1);
    if (ret != 1) {
        return HDF_FAILURE;
    }
    return HDF_SUCCESS;
}

int32_t dps310_init(struct dps310_dev *dev, DevHandle bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = DPS310_I2C_ADDR;

    /* Wait for sensor ready and coefficients ready */
    OsalMSleep(12);
    OsalMSleep(40);

    /* Read ID register */
    uint8_t id;
    if (dps310_write_then_read(dev, DPS310_REG_ID, &id, 1) != HDF_SUCCESS) {
        return HDF_FAILURE;
    }

    /* Soft reset */
    if (dps310_write(dev, DPS310_REG_RESET, 0x09) != HDF_SUCCESS) {
        return HDF_FAILURE;
    }

    /* Read calibration coefficients (required by test plan) */
    uint8_t coef_buf[18];
    if (dps310_write_then_read(dev, DPS310_REG_COEF, coef_buf, 18) != HDF_SUCCESS) {
        return HDF_FAILURE;
    }

    return HDF_SUCCESS;
}

int32_t dps310_read_pressure(struct dps310_dev *dev, int32_t *pressure_raw)
{
    uint8_t buf[3];
    if (dps310_write_then_read(dev, DPS310_REG_PSR_B2, buf, 3) != HDF_SUCCESS) {
        return HDF_FAILURE;
    }
    int32_t raw = ((int32_t)buf[0] << 16) | ((int32_t)buf[1] << 8) | (int32_t)buf[2];
    if (raw & 0x800000) {
        raw |= 0xFF000000;
    }
    *pressure_raw = raw;
    return HDF_SUCCESS;
}

int32_t dps310_read_temp(struct dps310_dev *dev, int32_t *temp_raw)
{
    uint8_t buf[3];
    if (dps310_write_then_read(dev, DPS310_REG_TMP_B2, buf, 3) != HDF_SUCCESS) {
        return HDF_FAILURE;
    }
    int32_t raw = ((int32_t)buf[0] << 16) | ((int32_t)buf[1] << 8) | (int32_t)buf[2];
    if (raw & 0x800000) {
        raw |= 0xFF000000;
    }
    *temp_raw = raw;
    return HDF_SUCCESS;
}
