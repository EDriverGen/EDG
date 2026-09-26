#include "lm75a.h"
#include <stddef.h>
#include <string.h>

#include "cmsis_rtx.h"
#define LM75A_I2C_ADDR 0x48
#define LM75A_REG_TEMP 0x00

int lm75a_init(struct lm75a_dev *dev, void *bus_handle)
{
    if (dev == NULL || bus_handle == NULL)
        return -1;
    dev->bus_handle = (I2C_HandleTypeDef *)bus_handle;
    dev->i2c_addr = LM75A_I2C_ADDR;
    return 0;
}

int lm75a_read_temperature(struct lm75a_dev *dev, int32_t *raw)
{
    uint8_t cmd = LM75A_REG_TEMP;
    uint8_t buf[2];
    HAL_StatusTypeDef ret;

    if (dev == NULL || dev->bus_handle == NULL || raw == NULL)
        return -1;

    // Write pointer register (0x00) then read 2 bytes using HAL_I2C_Mem_Read
    ret = HAL_I2C_Mem_Read(dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), cmd,
                           I2C_MEMADD_SIZE_8BIT, buf, 2, 100);
    if (ret != HAL_OK)
        return -1;

    // Combine bytes: big-endian, 16-bit raw
    int16_t raw16 = (int16_t)(((uint16_t)buf[0] << 8) | buf[1]);
    // Right-shift 5 to get 11-bit signed value, then sign-extend from bit 10
    int16_t raw11 = (int16_t)(raw16 >> 5);
    if (raw11 & 0x0400) // bit 10 set -> negative
        raw11 |= 0xF800; // sign extend to 16 bits
    else
        raw11 &= 0x07FF;

    // Convert to milli_degC: raw11 * 125000 (since 0.125 degC = 125 milli degC per LSB)
    // Use int64_t to avoid overflow
    int64_t temp_mdeg = (int64_t)raw11 * 125000;
    *raw = (int32_t)temp_mdeg;

    return 0;
}
