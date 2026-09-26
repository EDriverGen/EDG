#include "emc1413.h"
#include "stm32f1xx_hal.h"
#include "rtx_os.h"
#include "cmsis_os2.h"
#include <string.h>

#include "cmsis_rtx.h"
#define EMC1413_I2C_ADDR 0x4C
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

static int emc1413_write_then_read(struct emc1413_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint16_t addr = (uint16_t)(dev->i2c_addr << 1);
    if (HAL_I2C_Master_Transmit(hi2c, addr, &reg, 1, 100) != HAL_OK)
        return -1;
    if (HAL_I2C_Master_Receive(hi2c, addr, buf, len, 100) != HAL_OK)
        return -1;
    return 0;
}

static int emc1413_write(struct emc1413_dev *dev, uint8_t reg, uint8_t val)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint16_t addr = (uint16_t)(dev->i2c_addr << 1);
    uint8_t data[2] = {reg, val};
    if (HAL_I2C_Master_Transmit(hi2c, addr, data, 2, 100) != HAL_OK)
        return -1;
    return 0;
}

static int emc1413_read_temperature(struct emc1413_dev *dev, uint8_t reg_high, uint8_t reg_low, int32_t *temp)
{
    uint8_t high, low;
    if (emc1413_write_then_read(dev, reg_high, &high, 1) != 0)
        return -1;
    if (emc1413_write_then_read(dev, reg_low, &low, 1) != 0)
        return -1;
    *temp = ((int32_t)high * 1000) + ((((low >> 5) & 0x07) * 125));
    return 0;
}

int emc1413_init(struct emc1413_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = EMC1413_I2C_ADDR;

    osDelay(15);

    uint8_t id;
    if (emc1413_write_then_read(dev, EMC1413_REG_MANUFACTURER_ID, &id, 1) != 0)
        return -1;
    if (id != 0x5D)
        return -1;
    if (emc1413_write_then_read(dev, EMC1413_REG_PRODUCT_ID, &id, 1) != 0)
        return -1;
    if (id != 0x21)
        return -1;

    if (emc1413_write(dev, EMC1413_REG_CONFIG, 0x00) != 0)
        return -1;
    if (emc1413_write(dev, EMC1413_REG_CONV_RATE, 0x06) != 0)
        return -1;

    return 0;
}

int emc1413_read_internal_temperature(struct emc1413_dev *dev, int32_t *temp)
{
    return emc1413_read_temperature(dev, EMC1413_REG_INT_HIGH, EMC1413_REG_INT_LOW, temp);
}

int emc1413_read_external_diode_1_temperature(struct emc1413_dev *dev, int32_t *temp)
{
    return emc1413_read_temperature(dev, EMC1413_REG_EXT1_HIGH, EMC1413_REG_EXT1_LOW, temp);
}

int emc1413_read_external_diode_2_temperature(struct emc1413_dev *dev, int32_t *temp)
{
    return emc1413_read_temperature(dev, EMC1413_REG_EXT2_HIGH, EMC1413_REG_EXT2_LOW, temp);
}
