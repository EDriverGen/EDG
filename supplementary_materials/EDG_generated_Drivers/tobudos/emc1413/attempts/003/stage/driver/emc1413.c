#include "emc1413.h"
#include "stm32f1xx_hal.h"
#include "tos_k.h"
#include <stdint.h>
#include <stddef.h>

#include "tobudos.h"
#define EMC1413_I2C_ADDR 0x4C

static int emc1413_write_then_read(struct emc1413_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint16_t dev_addr = (uint16_t)(dev->i2c_addr << 1);
    if (HAL_I2C_Master_Transmit(hi2c, dev_addr, &reg, 1, 100) != HAL_OK)
        return -1;
    if (len > 0) {
        if (HAL_I2C_Master_Receive(hi2c, dev_addr, buf, len, 100) != HAL_OK)
            return -1;
    }
    return 0;
}

static int emc1413_write(struct emc1413_dev *dev, uint8_t reg, uint8_t val)
{
    I2C_HandleTypeDef *hi2c = (I2C_HandleTypeDef *)dev->bus_handle;
    uint16_t dev_addr = (uint16_t)(dev->i2c_addr << 1);
    uint8_t data[2] = {reg, val};
    if (HAL_I2C_Master_Transmit(hi2c, dev_addr, data, 2, 100) != HAL_OK)
        return -1;
    return 0;
}

int emc1413_init(struct emc1413_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = EMC1413_I2C_ADDR;

    tos_sleep_ms(15);

    uint8_t buf;
    if (emc1413_write_then_read(dev, 0xFE, &buf, 1) != 0) return -1;
    if (buf != 0x5D) return -1;
    if (emc1413_write_then_read(dev, 0xFD, &buf, 1) != 0) return -1;
    if (buf != 0x21) return -1;

    if (emc1413_write(dev, 0x03, 0x00) != 0) return -1;
    if (emc1413_write(dev, 0x04, 0x06) != 0) return -1;

    return 0;
}

static int emc1413_read_temperature(struct emc1413_dev *dev, uint8_t high_reg, uint8_t low_reg, int32_t *temp)
{
    uint8_t high, low;
    if (emc1413_write_then_read(dev, high_reg, &high, 1) != 0) return -1;
    if (emc1413_write_then_read(dev, low_reg, &low, 1) != 0) return -1;
    int32_t raw = ((int32_t)high << 8) | low;
    int32_t high_byte = (raw >> 8) & 0xFF;
    int32_t low_byte = raw & 0xFF;
    int32_t frac = (low_byte >> 5) & 0x07;
    *temp = (high_byte * 1000) + (frac * 125);
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
