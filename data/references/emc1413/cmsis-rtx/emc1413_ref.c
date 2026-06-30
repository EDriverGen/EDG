#include "emc1413_ref.h"

static int emc1413_read_register(struct emc1413_device *dev, uint8_t reg,
                                 uint8_t *value)
{
    if (dev == 0 || dev->bus == 0 || value == 0) {
        return -1;
    }
    return HAL_I2C_Mem_Read(dev->bus, (uint16_t)(dev->addr << 1), reg,
                            I2C_MEMADD_SIZE_8BIT, value, 1, 100) == HAL_OK
        ? 0 : -1;
}

static int emc1413_write_register(struct emc1413_device *dev, uint8_t reg,
                                  uint8_t value)
{
    if (dev == 0 || dev->bus == 0) {
        return -1;
    }
    return HAL_I2C_Mem_Write(dev->bus, (uint16_t)(dev->addr << 1), reg,
                             I2C_MEMADD_SIZE_8BIT, &value, 1, 100) == HAL_OK
        ? 0 : -1;
}

static int emc1413_channel_regs(enum emc1413_channel channel, uint8_t *hi, uint8_t *lo)
{
    if (hi == 0 || lo == 0) {
        return -1;
    }
    switch (channel) {
    case EMC1413_CH_INTERNAL:
        *hi = EMC1413_REG_INTERNAL_TEMP_HI;
        *lo = EMC1413_REG_INTERNAL_TEMP_LO;
        return 0;
    case EMC1413_CH_EXTERNAL_1:
        *hi = EMC1413_REG_EXT1_TEMP_HI;
        *lo = EMC1413_REG_EXT1_TEMP_LO;
        return 0;
    case EMC1413_CH_EXTERNAL_2:
        *hi = EMC1413_REG_EXT2_TEMP_HI;
        *lo = EMC1413_REG_EXT2_TEMP_LO;
        return 0;
    default:
        return -1;
    }
}

static int32_t emc1413_raw_to_mcelsius(uint8_t hi, uint8_t lo)
{
    int32_t integer = (int8_t)hi;
    int32_t frac = (int32_t)(((lo >> 5) & 0x07U) * 125U);
    return integer * 1000 + frac;
}

int emc1413_init(struct emc1413_device *dev, I2C_HandleTypeDef *bus, uint8_t addr)
{
    if (dev == 0 || bus == 0 || addr == 0) {
        return -1;
    }
    if (HAL_I2C_Init(bus) != HAL_OK) {
        return -1;
    }
    dev->bus = bus;
    dev->addr = addr;
    return 0;
}

int emc1413_probe(struct emc1413_device *dev)
{
    uint8_t mfr = 0;
    if (emc1413_read_register(dev, EMC1413_REG_MANUFACTURER_ID, &mfr) != 0) {
        return -1;
    }
    return mfr == EMC1413_MANUFACTURER_ID ? 0 : -1;
}

int emc1413_read_temperature(struct emc1413_device *dev, enum emc1413_channel channel,
                             int32_t *temp_mcelsius)
{
    uint8_t hi_reg = 0;
    uint8_t lo_reg = 0;
    uint8_t hi = 0;
    uint8_t lo = 0;
    if (temp_mcelsius == 0 || emc1413_channel_regs(channel, &hi_reg, &lo_reg) != 0) {
        return -1;
    }
    if (emc1413_read_register(dev, hi_reg, &hi) != 0 ||
        emc1413_read_register(dev, lo_reg, &lo) != 0) {
        return -1;
    }
    *temp_mcelsius = emc1413_raw_to_mcelsius(hi, lo);
    return 0;
}

int emc1413_set_extended_range(struct emc1413_device *dev, int enable)
{
    uint8_t cfg = 0;
    if (emc1413_read_register(dev, EMC1413_REG_CONFIG, &cfg) != 0) {
        return -1;
    }
    if (enable) {
        cfg |= EMC1413_CONFIG_RANGE;
    } else {
        cfg &= (uint8_t)~EMC1413_CONFIG_RANGE;
    }
    return emc1413_write_register(dev, EMC1413_REG_CONFIG, cfg);
}
