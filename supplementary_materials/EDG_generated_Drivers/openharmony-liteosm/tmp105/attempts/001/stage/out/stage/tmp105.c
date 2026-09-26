#include "tmp105.h"
#include "i2c_msg.h"
#include "hdf_base.h"
#include "osal_time.h"
#include <string.h>

#include "openharmony_liteosm.h"
#define TMP105_I2C_ADDR 0x48
#define TMP105_PTR_TEMP 0x00

int32_t tmp105_init(struct tmp105_dev *dev, DevHandle bus_handle)
{
    if (dev == NULL || bus_handle == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    dev->bus_handle = bus_handle;
    dev->i2c_addr = TMP105_I2C_ADDR;
    return HDF_SUCCESS;
}

int32_t tmp105_read_temperature(struct tmp105_dev *dev, int32_t *raw)
{
    if (dev == NULL || raw == NULL || dev->bus_handle == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }

    // Wait for conversion time (12-bit resolution typical 220ms)
    OsalMSleep(220);

    // Write pointer register to select Temperature Register
    uint8_t ptr = TMP105_PTR_TEMP;
    struct I2cMsg msgs[2];
    int16_t count = 2;

    // Write message
    msgs[0].addr = dev->i2c_addr;
    msgs[0].buf = &ptr;
    msgs[0].len = 1;
    msgs[0].flags = 0; // write

    // Read message
    uint8_t buf[2];
    msgs[1].addr = dev->i2c_addr;
    msgs[1].buf = buf;
    msgs[1].len = 2;
    msgs[1].flags = I2C_FLAG_READ; // read

    int32_t ret = I2cTransfer(dev->bus_handle, msgs, count);
    if (ret != count) {
        return HDF_FAILURE;
    }

    // Decode raw value: big-endian, 12-bit, signed
    uint16_t raw16 = ((uint16_t)buf[0] << 8) | buf[1];
    int16_t raw12 = (int16_t)(raw16 >> 4);
    // Sign extend from bit 11
    if (raw12 & 0x0800) {
        raw12 |= 0xF000;
    }
    // Convert to milli_degC: (raw12 * 625) / 10
    int32_t milli = ((int32_t)raw12 * 625) / 10;
    *raw = milli;
    return HDF_SUCCESS;
}
