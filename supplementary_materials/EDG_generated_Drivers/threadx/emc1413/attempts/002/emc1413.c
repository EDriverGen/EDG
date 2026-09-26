#include "emc1413.h"
#include "stm32f1xx_hal_i2c.h"
#include "tx_api.h"
#include <stdint.h>

#include "threadx.h"
static int emc1413_write_then_read(struct emc1413_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    HAL_StatusTypeDef ret;
    ret = HAL_I2C_Master_Transmit(dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), &reg, 1, 100);
    if (ret != HAL_OK) return -1;
    ret = HAL_I2C_Master_Receive(dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), buf, len, 100);
    if (ret != HAL_OK) return -1;
    return 0;
}

static int emc1413_write(struct emc1413_dev *dev, uint8_t reg, uint8_t val)
{
    uint8_t data[2] = {reg, val};
    HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), data, 2, 100);
    if (ret != HAL_OK) return -1;
    return 0;
}

int emc1413_init(struct emc1413_dev *dev, void *bus_handle)
{
    dev->bus_handle = (I2C_HandleTypeDef *)bus_handle;
    dev->i2c_addr = EMC1413_I2C_ADDR;

    tx_thread_sleep(15);

    uint8_t id;
    if (emc1413_write_then_read(dev, 0xFE, &id, 1) != 0) return -1;
    if (id != 0x5D) return -1;
    if (emc1413_write_then_read(dev, 0xFD, &id, 1) != 0) return -1;
    if (id != 0x21) return -1;

    if (emc1413_write(dev, 0x03, 0x00) != 0) return -1;
    if (emc1413_write(dev, 0x04, 0x06) != 0) return -1;

    return 0;
}

static int emc1413_read_temperature(struct emc1413_dev *dev, uint8_t high_reg, uint8_t low_reg, int32_t *temp)
{
    uint8_t high, low;
    if (emc1413_write_then_read(dev, high_reg, &high, 1) != 0) return -1;
    if (emc1413_write_then_read(dev, low_reg, &low, 1) != 0) return -1;
    *temp = ((int32_t)high * 1000) + (((int32_t)((low >> 5) & 0x07)) * 125);
    return 0;
}

int emc1413_read_internal_temperature(struct emc1413_dev *dev, int32_t *temp)
{
    return emc1413_read_temperature(dev, 0x00, 0x29, temp);
}

int emc1413_read_external_diode_1_temperature(struct emc1413_dev *dev, int32_t *temp)
{
    return emc1413_read_temperature(dev, 0x01, 0x10, temp);
}

int emc1413_read_external_diode_2_temperature(struct emc1413_dev *dev, int32_t *temp)
{
    return emc1413_read_temperature(dev, 0x23, 0x24, temp);
}
