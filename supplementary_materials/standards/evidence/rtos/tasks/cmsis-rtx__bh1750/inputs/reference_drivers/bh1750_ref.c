#include "bh1750_ref.h"

#define BH1750_CMD_POWER_DOWN 0x00
#define BH1750_CMD_POWER_ON   0x01
#define BH1750_CMD_RESET      0x07

static int cmsis_rtx_i2c_write(I2C_HandleTypeDef *bus, uint16_t addr,
                               const uint8_t *data, uint16_t len)
{
    if (bus == 0 || data == 0) {
        return -1;
    }
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(
        bus, (uint16_t)(addr << 1), (uint8_t *)data, len, 100
    );
    return status == HAL_OK ? 0 : -1;
}

static int cmsis_rtx_i2c_read(I2C_HandleTypeDef *bus, uint16_t addr,
                              uint8_t *data, uint16_t len)
{
    if (bus == 0 || data == 0) {
        return -1;
    }
    HAL_StatusTypeDef status = HAL_I2C_Master_Receive(
        bus, (uint16_t)(addr << 1), data, len, 100
    );
    return status == HAL_OK ? 0 : -1;
}

static int bh1750_write_cmd(struct bh1750_device *dev, uint8_t cmd)
{
    if (dev == 0 || dev->bus == 0) {
        return -1;
    }
    return cmsis_rtx_i2c_write(dev->bus, dev->addr, &cmd, 1);
}

int bh1750_init(struct bh1750_device *dev, I2C_HandleTypeDef *bus, uint16_t addr)
{
    if (dev == 0 || bus == 0) {
        return -1;
    }
    if (addr != BH1750_ADDR_LOW && addr != BH1750_ADDR_HIGH) {
        return -1;
    }
    dev->bus = bus;
    dev->addr = addr;
    dev->mode = BH1750_ONE_H_RES_MODE;
    return 0;
}

int bh1750_probe(struct bh1750_device *dev)
{
    int ret = bh1750_write_cmd(dev, BH1750_CMD_POWER_ON);
    if (ret != 0) {
        return ret;
    }
    return bh1750_write_cmd(dev, BH1750_CMD_POWER_DOWN);
}

int bh1750_read_raw(struct bh1750_device *dev, uint16_t *raw)
{
    uint8_t data[2];
    int ret;

    if (dev == 0 || raw == 0) {
        return -1;
    }

    ret = bh1750_write_cmd(dev, BH1750_CMD_POWER_ON);
    if (ret != 0) {
        return ret;
    }
    ret = bh1750_write_cmd(dev, BH1750_CMD_RESET);
    if (ret != 0) {
        return ret;
    }
    ret = bh1750_write_cmd(dev, dev->mode);
    if (ret != 0) {
        return ret;
    }

    (void)osDelay(180);

    ret = cmsis_rtx_i2c_read(dev->bus, dev->addr, data, 2);
    if (ret != 0) {
        return ret;
    }
    *raw = (uint16_t)(((uint16_t)data[0] << 8) | data[1]);
    return 0;
}

uint32_t bh1750_raw_to_lux_x100(uint16_t raw)
{
    return (uint32_t)raw * 1000U / 12U;
}

int bh1750_read_lux_x100(struct bh1750_device *dev, uint32_t *lux_x100)
{
    uint16_t raw;
    int ret;
    if (lux_x100 == 0) {
        return -1;
    }
    ret = bh1750_read_raw(dev, &raw);
    if (ret != 0) {
        return ret;
    }
    *lux_x100 = bh1750_raw_to_lux_x100(raw);
    return 0;
}
