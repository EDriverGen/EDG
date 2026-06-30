#include "dps310_ref.h"

static int dps310_read_registers(struct dps310_device *dev, uint8_t reg, uint8_t *buffer, uint16_t size)
{
    if (dev == 0 || dev->bus == 0 || buffer == 0 || size == 0) return -1;
    return HAL_I2C_Mem_Read(dev->bus, (uint16_t)(dev->addr << 1), reg,
                            I2C_MEMADD_SIZE_8BIT, buffer, size, 100) == HAL_OK ? 0 : -1;
}

static int dps310_write_register(struct dps310_device *dev, uint8_t reg, uint8_t value)
{
    if (dev == 0 || dev->bus == 0) return -1;
    return HAL_I2C_Mem_Write(dev->bus, (uint16_t)(dev->addr << 1), reg,
                             I2C_MEMADD_SIZE_8BIT, &value, 1, 100) == HAL_OK ? 0 : -1;
}

static void dps310_delay_ms(uint32_t ms)
{
    (void)osDelay(ms);
}

int dps310_init(struct dps310_device *dev, I2C_HandleTypeDef *bus, uint8_t addr)
{
    if (dev == 0 || bus == 0 || addr == 0) return -1;
    if (HAL_I2C_Init(bus) != HAL_OK) return -1;
    dev->bus = bus;
    dev->addr = addr;
    dev->kT = DPS310_SCALE_FACTOR_1;
    dev->kP = DPS310_SCALE_FACTOR_1;
    return 0;
}

#include "dps310_common_body.h"
