#include "tmp421_ref.h"

static int tmp421_read_register(struct tmp421_device *dev, uint8_t reg,
                                uint8_t *value)
{
    if (dev == 0 || dev->bus == 0 || value == 0) {
        return -1;
    }
    return HAL_I2C_Mem_Read(dev->bus, (uint16_t)(dev->addr << 1), reg,
                            I2C_MEMADD_SIZE_8BIT, value, 1, 100) == HAL_OK
        ? 0 : -1;
}

static int tmp421_write_register(struct tmp421_device *dev, uint8_t reg,
                                 uint8_t value)
{
    if (dev == 0 || dev->bus == 0) {
        return -1;
    }
    return HAL_I2C_Mem_Write(dev->bus, (uint16_t)(dev->addr << 1), reg,
                             I2C_MEMADD_SIZE_8BIT, &value, 1, 100) == HAL_OK
        ? 0 : -1;
}

static int32_t tmp421_raw_to_mcelsius(uint8_t hi, uint8_t lo)
{
    int16_t raw = ((int16_t)(((uint16_t)hi << 8) | lo)) >> 4;
    return (int32_t)raw * 625 / 10;
}

int tmp421_init(struct tmp421_device *dev, I2C_HandleTypeDef *bus, uint8_t addr)
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

int tmp421_probe(struct tmp421_device *dev)
{
    uint8_t mfr = 0;
    if (tmp421_read_register(dev, TMP421_REG_MANUFACTURER_ID, &mfr) != 0) {
        return -1;
    }
    return mfr == TMP421_MANUFACTURER_ID_TI ? 0 : -1;
}

int tmp421_read_local_temp(struct tmp421_device *dev, int32_t *temp_mcelsius)
{
    uint8_t hi = 0;
    uint8_t lo = 0;
    if (temp_mcelsius == 0) {
        return -1;
    }
    if (tmp421_read_register(dev, TMP421_REG_LOCAL_TEMP_HI, &hi) != 0 ||
        tmp421_read_register(dev, TMP421_REG_LOCAL_TEMP_LO, &lo) != 0) {
        return -1;
    }
    *temp_mcelsius = tmp421_raw_to_mcelsius(hi, lo);
    return 0;
}

int tmp421_read_remote_temp(struct tmp421_device *dev, int32_t *temp_mcelsius)
{
    uint8_t hi = 0;
    uint8_t lo = 0;
    if (temp_mcelsius == 0) {
        return -1;
    }
    if (tmp421_read_register(dev, TMP421_REG_REMOTE_TEMP_HI, &hi) != 0 ||
        tmp421_read_register(dev, TMP421_REG_REMOTE_TEMP_LO, &lo) != 0) {
        return -1;
    }
    *temp_mcelsius = tmp421_raw_to_mcelsius(hi, lo);
    return 0;
}

int tmp421_set_extended_range(struct tmp421_device *dev, int enable)
{
    uint8_t cfg = 0;
    if (tmp421_read_register(dev, TMP421_REG_CONFIG_1, &cfg) != 0) {
        return -1;
    }
    if (enable) {
        cfg |= TMP421_CONFIG1_RANGE;
    } else {
        cfg &= (uint8_t)~TMP421_CONFIG1_RANGE;
    }
    return tmp421_write_register(dev, TMP421_REG_CONFIG_1_WR, cfg);
}
