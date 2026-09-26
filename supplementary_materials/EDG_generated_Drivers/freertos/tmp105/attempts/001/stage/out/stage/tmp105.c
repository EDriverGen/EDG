#include "tmp105.h"
#include <stddef.h>
#include <stdint.h>
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_i2c.h"
#include "projdefs.h"
#include "task.h"

#include "freertos.h"
#define TMP105_I2C_ADDR 0x48
#define TMP105_PTR_TEMP 0x00

int tmp105_init(struct tmp105_dev *dev, void *bus_handle)
{
    if (dev == NULL || bus_handle == NULL) {
        return -1;
    }
    dev->bus_handle = (I2C_HandleTypeDef *)bus_handle;
    dev->i2c_addr = TMP105_I2C_ADDR;
    return 0;
}

int tmp105_read_temperature(struct tmp105_dev *dev, int32_t *raw)
{
    uint8_t cmd = TMP105_PTR_TEMP;
    uint8_t buf[2];
    HAL_StatusTypeDef ret;

    if (dev == NULL || raw == NULL) {
        return -1;
    }

    // Wait for conversion time (12-bit resolution typical 220ms)
    vTaskDelay(pdMS_TO_TICKS(220));

    // Write pointer then read 2 bytes using HAL_I2C_Mem_Read
    ret = HAL_I2C_Mem_Read(dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), cmd, I2C_MEMADD_SIZE_8BIT, buf, 2, 100);
    if (ret != HAL_OK) {
        return -1;
    }

    // Decode: big-endian, 12-bit two's complement
    uint16_t raw16 = ((uint16_t)buf[0] << 8) | buf[1];
    int16_t raw12 = (int16_t)(raw16 >> 4);
    // Sign extend from bit 11
    if (raw12 & 0x0800) {
        raw12 |= 0xF000;
    }
    // Convert to milli_degC: raw12 * 625 / 10
    int32_t milli = ((int32_t)raw12 * 625) / 10;
    *raw = milli;
    return 0;
}
