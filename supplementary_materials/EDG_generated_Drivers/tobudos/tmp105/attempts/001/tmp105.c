#include "tmp105.h"
#include "tos_k.h"
#include <stddef.h>

#include "tobudos.h"
#define TMP105_I2C_ADDR 0x48
#define TMP105_PTR_TEMP 0x00

static int tmp105_write_then_read(struct tmp105_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    HAL_StatusTypeDef ret;
    ret = HAL_I2C_Master_Transmit(dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), &reg, 1, 100);
    if (ret != HAL_OK) return -1;
    ret = HAL_I2C_Master_Receive(dev->bus_handle, (uint16_t)(dev->i2c_addr << 1), buf, len, 100);
    if (ret != HAL_OK) return -1;
    return 0;
}

int tmp105_init(struct tmp105_dev *dev, void *bus_handle)
{
    if (!dev || !bus_handle) return -1;
    dev->bus_handle = (I2C_HandleTypeDef *)bus_handle;
    dev->i2c_addr = TMP105_I2C_ADDR;
    return 0;
}

int tmp105_read_temperature(struct tmp105_dev *dev, int32_t *raw)
{
    uint8_t buf[2];
    int ret;
    if (!dev || !raw) return -1;
    tos_sleep_ms(220);
    ret = tmp105_write_then_read(dev, TMP105_PTR_TEMP, buf, 2);
    if (ret != 0) return -1;
    uint16_t raw16 = ((uint16_t)buf[0] << 8) | buf[1];
    int16_t raw12 = (int16_t)(raw16 >> 4);
    if (raw12 & 0x0800) {
        raw12 |= 0xF000;
    }
    *raw = ((int32_t)raw12 * 625) / 10;
    return 0;
}
