#include "emc1413.h"
#include "tos_k.h"
#include <stddef.h>

#include "tobudos.h"
#define EMC1413_REG_MANUFACTURER_ID 0xFE
#define EMC1413_REG_PRODUCT_ID 0xFD
#define EMC1413_REG_CONFIG 0x03
#define EMC1413_REG_CONV_RATE 0x04
#define EMC1413_REG_INT_HIGH 0x00
#define EMC1413_REG_INT_LOW 0x29
#define EMC1413_REG_EXT1_HIGH 0x01
#define EMC1413_REG_EXT1_LOW 0x10
#define EMC1413_REG_EXT2_HIGH 0x23
#define EMC1413_REG_EXT2_LOW 0x24

static int emc1413_write_reg(struct emc1413_dev *dev, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), buf, 2, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

static int emc1413_read_reg(struct emc1413_dev *dev, uint8_t reg, uint8_t *val)
{
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), &reg, 1, 100);
    if (ret != HAL_OK) return -1;
    ret = HAL_I2C_Master_Receive(dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), val, 1, 100);
    return (ret == HAL_OK) ? 0 : -1;
}

static int emc1413_read_temperature(struct emc1413_dev *dev, uint8_t high_reg, uint8_t low_reg, int32_t *temp_milli)
{
    uint8_t high, low;
    if (emc1413_read_reg(dev, high_reg, &high) != 0) return -1;
    if (emc1413_read_reg(dev, low_reg, &low) != 0) return -1;
    int32_t frac = (low >> 5) & 0x07;
    *temp_milli = ((int32_t)high * 1000) + (frac * 125);
    return 0;
}

int emc1413_init(struct emc1413_dev *dev, void *bus_handle)
{
    dev->bus_handle = (I2C_HandleTypeDef *)bus_handle;
    dev->i2c_addr = EMC1413_I2C_ADDR;

    tos_sleep_ms(15);

    uint8_t id;
    if (emc1413_read_reg(dev, EMC1413_REG_MANUFACTURER_ID, &id) != 0) return -1;
    if (id != 0x5D) return -1;
    if (emc1413_read_reg(dev, EMC1413_REG_PRODUCT_ID, &id) != 0) return -1;
    if (id != 0x21) return -1;

    if (emc1413_write_reg(dev, EMC1413_REG_CONFIG, 0x00) != 0) return -1;
    if (emc1413_write_reg(dev, EMC1413_REG_CONV_RATE, 0x06) != 0) return -1;

    return 0;
}

int emc1413_read_internal_temperature(struct emc1413_dev *dev, int32_t *temp_local_val)
{
    return emc1413_read_temperature(dev, EMC1413_REG_INT_HIGH, EMC1413_REG_INT_LOW, temp_local_val);
}

int emc1413_read_external_diode_1_temperature(struct emc1413_dev *dev, int32_t *temp_ext1_val)
{
    return emc1413_read_temperature(dev, EMC1413_REG_EXT1_HIGH, EMC1413_REG_EXT1_LOW, temp_ext1_val);
}

int emc1413_read_external_diode_2_temperature(struct emc1413_dev *dev, int32_t *temp_ext2_val)
{
    return emc1413_read_temperature(dev, EMC1413_REG_EXT2_HIGH, EMC1413_REG_EXT2_LOW, temp_ext2_val);
}
