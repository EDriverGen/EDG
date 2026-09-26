#include "tmp105.h"
#include "stm32f1xx_hal.h"
#include "tx_api.h"

#include "threadx.h"
#define TMP105_I2C_ADDR 0x48
#define TMP105_PTR_TEMP 0x00

static int tmp105_write_then_read(struct tmp105_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len)
{
    HAL_StatusTypeDef ret;
    ret = HAL_I2C_Mem_Read((I2C_HandleTypeDef *)dev->bus_handle,
                           (uint16_t)(dev->i2c_addr << 1),
                           reg,
                           I2C_MEMADD_SIZE_8BIT,
                           buf,
                           len,
                           100);
    return (ret == HAL_OK) ? 0 : -1;
}

int tmp105_init(struct tmp105_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = TMP105_I2C_ADDR;
    return 0;
}

int tmp105_read_temperature(struct tmp105_dev *dev, int32_t *raw)
{
    uint8_t buf[2];
    int ret;
    int16_t raw12;
    int32_t milli;

    tx_thread_sleep(220);

    ret = tmp105_write_then_read(dev, TMP105_PTR_TEMP, buf, 2);
    if (ret != 0) {
        return -1;
    }

    raw12 = (int16_t)(((uint16_t)buf[0] << 8) | buf[1]);
    raw12 >>= 4;
    if (raw12 & 0x0800) {
        raw12 |= 0xF000;
    }

    milli = ((int32_t)raw12 * 625) / 10;
    *raw = milli;
    return 0;
}
