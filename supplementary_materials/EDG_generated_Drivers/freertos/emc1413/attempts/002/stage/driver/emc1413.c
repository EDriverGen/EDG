#include "emc1413.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_i2c.h"
#include "task.h"
#include <stdint.h>
#include <stddef.h>

#include "freertos.h"
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

static int emc1413_write_reg(struct emc1413_dev *dev, uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = {reg, data};
    if (HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), buf, 2, 100) != HAL_OK)
        return -1;
    return 0;
}

static int emc1413_read_reg(struct emc1413_dev *dev, uint8_t reg, uint8_t *data)
{
    if (HAL_I2C_Master_Transmit((I2C_HandleTypeDef *)dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), &reg, 1, 100) != HAL_OK)
        return -1;
    if (HAL_I2C_Master_Receive((I2C_HandleTypeDef *)dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), data, 1, 100) != HAL_OK)
        return -1;
    return 0;
}

static int emc1413_read_temperature(struct emc1413_dev *dev, uint8_t reg_high, uint8_t reg_low, int32_t *temp_milli)
{
    uint8_t high, low;
    if (emc1413_read_reg(dev, reg_high, &high) != 0)
        return -1;
    if (emc1413_read_reg(dev, reg_low, &low) != 0)
        return -1;
    int32_t raw = ((int32_t)high << 8) | low;
    int32_t high_byte = (raw >> 8) & 0xFF;
    int32_t low_byte = raw & 0xFF;
    int32_t frac = (low_byte >> 5) & 0x07;
    *temp_milli = (high_byte * 1000) + (frac * 125);
    return 0;
}

int emc1413_init(struct emc1413_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = EMC1413_I2C_ADDR;

    vTaskDelay(pdMS_TO_TICKS(15));

    uint8_t id;
    if (emc1413_read_reg(dev, EMC1413_REG_MANUFACTURER_ID, &id) != 0)
        return -1;
    if (id != 0x5D)
        return -1;
    if (emc1413_read_reg(dev, EMC1413_REG_PRODUCT_ID, &id) != 0)
        return -1;
    if (id != 0x21)
        return -1;

    if (emc1413_write_reg(dev, EMC1413_REG_CONFIG, 0x00) != 0)
        return -1;
    if (emc1413_write_reg(dev, EMC1413_REG_CONV_RATE, 0x06) != 0)
        return -1;

    return 0;
}

int emc1413_read_internal_temperature(struct emc1413_dev *dev, int32_t *temp_local)
{
    return emc1413_read_temperature(dev, EMC1413_REG_INT_HIGH, EMC1413_REG_INT_LOW, temp_local);
}

int emc1413_read_external_diode_1_temperature(struct emc1413_dev *dev, int32_t *temp_ext1)
{
    return emc1413_read_temperature(dev, EMC1413_REG_EXT1_HIGH, EMC1413_REG_EXT1_LOW, temp_ext1);
}

int emc1413_read_external_diode_2_temperature(struct emc1413_dev *dev, int32_t *temp_ext2)
{
    return emc1413_read_temperature(dev, EMC1413_REG_EXT2_HIGH, EMC1413_REG_EXT2_LOW, temp_ext2);
}
