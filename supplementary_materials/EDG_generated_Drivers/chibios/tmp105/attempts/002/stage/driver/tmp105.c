#include "tmp105.h"
#include "ch.h"
#include "hal.h"
#include "hal_i2c_lld.h"
#include "hal_i2c.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "chibios.h"
#define TMP105_I2C_ADDR 0x48
#define TMP105_PTR_TEMP 0x00
#define TMP105_CONVERSION_DELAY_MS 220

static int tmp105_write_then_read(struct tmp105_dev *dev, uint8_t reg, uint8_t *rxbuf, size_t rxlen)
{
    I2CDriver *i2cp = (I2CDriver *)dev->bus_handle;
    i2cAcquireBus(i2cp);
    msg_t ret = i2cMasterTransmitTimeout(i2cp, TMP105_I2C_ADDR, &reg, 1, rxbuf, rxlen, TIME_MS2I(100));
    i2cReleaseBus(i2cp);
    return (ret == MSG_OK) ? 0 : -1;
}

int tmp105_init(struct tmp105_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = TMP105_I2C_ADDR;
    return 0;
}

int tmp105_read_temperature(struct tmp105_dev *dev, int32_t *raw)
{
    uint8_t buf[2];
    int ret;

    chThdSleepMilliseconds(TMP105_CONVERSION_DELAY_MS);

    ret = tmp105_write_then_read(dev, TMP105_PTR_TEMP, buf, 2);
    if (ret != 0) {
        return ret;
    }

    uint16_t raw16 = ((uint16_t)buf[0] << 8) | buf[1];
    int16_t raw12 = (int16_t)(raw16 >> 4);
    if (raw12 & 0x0800) {
        raw12 |= 0xF000;
    }
    *raw = ((int32_t)raw12 * 125) / 2;
    return 0;
}
