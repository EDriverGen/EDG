#include "tmp105.h"
#include "cmsis_os2.h"
#include <stddef.h>

#include "cmsis_rtx.h"
#define TMP105_I2C_ADDR 0x48
#define TMP105_PTR_TEMP 0x00

int tmp105_init(struct tmp105_dev *dev, void *bus_handle)
{
    if (dev == NULL || bus_handle == NULL) return -1;
    dev->bus_handle = (I2C_HandleTypeDef *)bus_handle;
    dev->i2c_addr = TMP105_I2C_ADDR;
    return 0;
}

int tmp105_read_temperature(struct tmp105_dev *dev, int32_t *raw)
{
    if (dev == NULL || dev->bus_handle == NULL || raw == NULL) return -1;

    uint8_t cmd = TMP105_PTR_TEMP;
    uint8_t buf[2];
    HAL_StatusTypeDef ret;

    // Write pointer then read 2 bytes using HAL_I2C_Mem_Read
    ret = HAL_I2C_Mem_Read(dev->bus_handle, (uint16_t)(dev->i2c_addr << 1),
                           (uint16_t)cmd, I2C_MEMADD_SIZE_8BIT,
                           buf, 2, 100);
    if (ret != HAL_OK) return -1;

    // Combine bytes (big-endian)
    int16_t raw16 = (int16_t)(((uint16_t)buf[0] << 8) | buf[1]);
    // Right-shift 4, mask 12 bits, sign-extend from bit 11
    uint16_t uraw = (uint16_t)((raw16 >> 4) & 0x0FFF);
    int16_t sraw;
    if (uraw & 0x0800) {
        sraw = (int16_t)(uraw | 0xF000);
    } else {
        sraw = (int16_t)uraw;
    }
    // Convert to milli_degC: (sraw * 625) / 10
    int32_t milli = ((int32_t)sraw * 625) / 10;
    *raw = milli;
    return 0;
}
