/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * EMC1413 Temperature Sensor Driver for ChibiOS
 */
#include "emc1413_ref.h"

static int emc1413_read_reg(struct emc1413_device *dev,
                               uint8_t reg, uint8_t *buf, uint16_t len)
{
    msg_t ret;
    if (dev == NULL || dev->bus == NULL || buf == NULL) return -1;
    i2cAcquireBus(dev->bus);
    ret = i2cMasterTransmitTimeout(dev->bus, dev->addr,
                                   &reg, 1, buf, len, TIME_MS2I(100));
    i2cReleaseBus(dev->bus);
    return (ret == MSG_OK) ? 0 : -1;
}

/*
 * EMC1413 encodes every temperature channel (internal and the two
 * external channels) as a signed 11-bit value split across an integer
 * high byte and a fractional low byte (bits [7:5] = 0.125 C steps).
 * Build a signed int16 and arithmetic-shift right by 5 so the sign
 * survives for negative temperatures (e.g. 0xEC00 >> 5 = -160 -> -20 C).
 */
static int32_t emc1413_ext_raw_to_mcelsius(uint8_t msb, uint8_t lsb)
{
    int16_t raw = (int16_t)((msb << 8) | lsb);
    return ((int32_t)(raw >> 5) * 125);
}

int emc1413_init(struct emc1413_device *dev, I2CDriver *bus, uint16_t addr)
{
    if (dev == NULL) return -1;
    dev->bus  = bus;
    dev->addr = addr;
    return 0;
}

int emc1413_probe(struct emc1413_device *dev)
{
    uint8_t id;
    int ret = emc1413_read_reg(dev, EMC1413_REG_MFG_ID, &id, 1);
    if (ret != 0) return ret;
    if (id != EMC1413_MFG_ID_EXPECTED) return -3;
    return 0;
}

int emc1413_read_internal_temp(struct emc1413_device *dev, int32_t *temp_mcelsius)
{
    uint8_t msb, lsb;
    int ret;
    if (dev == NULL || temp_mcelsius == NULL) return -1;
    /*
     * Read the integer byte at 0x00 and the fractional extension byte
     * at 0x29 so the reading carries full 0.125 C resolution. Using the
     * shared signed decoder keeps the algorithm identical to the
     * external channels.
     */
    ret = emc1413_read_reg(dev, EMC1413_REG_INTERNAL_TEMP, &msb, 1);
    if (ret != 0) return ret;
    ret = emc1413_read_reg(dev, EMC1413_REG_INTERNAL_TEMP_L, &lsb, 1);
    if (ret != 0) return ret;
    *temp_mcelsius = emc1413_ext_raw_to_mcelsius(msb, lsb);
    return 0;
}

int emc1413_read_external1_temp(struct emc1413_device *dev, int32_t *temp_mcelsius)
{
    uint8_t msb, lsb;
    int ret;
    if (dev == NULL || temp_mcelsius == NULL) return -1;
    ret = emc1413_read_reg(dev, EMC1413_REG_EXT1_TEMP_H, &msb, 1);
    if (ret != 0) return ret;
    ret = emc1413_read_reg(dev, EMC1413_REG_EXT1_TEMP_L, &lsb, 1);
    if (ret != 0) return ret;
    *temp_mcelsius = emc1413_ext_raw_to_mcelsius(msb, lsb);
    return 0;
}

int emc1413_read_external2_temp(struct emc1413_device *dev, int32_t *temp_mcelsius)
{
    uint8_t msb, lsb;
    int ret;
    if (dev == NULL || temp_mcelsius == NULL) return -1;
    ret = emc1413_read_reg(dev, EMC1413_REG_EXT2_TEMP_H, &msb, 1);
    if (ret != 0) return ret;
    ret = emc1413_read_reg(dev, EMC1413_REG_EXT2_TEMP_L, &lsb, 1);
    if (ret != 0) return ret;
    *temp_mcelsius = emc1413_ext_raw_to_mcelsius(msb, lsb);
    return 0;
}
