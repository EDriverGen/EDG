#include "mcp23017.h"
#include "i2c_msg.h"
#include "hdf_base.h"
#include "osal_time.h"
#include <stdint.h>

#include "openharmony_liteosm.h"
#define MCP23017_IODIRA 0x00
#define MCP23017_IODIRB 0x01
#define MCP23017_GPIOA  0x12
#define MCP23017_GPIOB  0x13

static int mcp23017_write_reg(struct mcp23017_dev *dev, uint8_t reg, uint8_t data)
{
    struct I2cMsg msgs[1];
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = data;
    msgs[0].addr = dev->i2c_addr;
    msgs[0].buf = buf;
    msgs[0].len = 2;
    msgs[0].flags = 0;
    int32_t ret = I2cTransfer(dev->bus_handle, msgs, 1);
    if (ret != 1) {
        return -1;
    }
    return 0;
}

static int mcp23017_read_reg(struct mcp23017_dev *dev, uint8_t reg, uint8_t *data)
{
    struct I2cMsg msgs[2];
    uint8_t reg_buf = reg;
    msgs[0].addr = dev->i2c_addr;
    msgs[0].buf = &reg_buf;
    msgs[0].len = 1;
    msgs[0].flags = 0;
    msgs[1].addr = dev->i2c_addr;
    msgs[1].buf = data;
    msgs[1].len = 1;
    msgs[1].flags = 0x01; /* I2C_FLAG_READ */
    int32_t ret = I2cTransfer(dev->bus_handle, msgs, 2);
    if (ret != 2) {
        return -1;
    }
    return 0;
}

int mcp23017_init(struct mcp23017_dev *dev, DevHandle bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = MCP23017_I2C_ADDR;
    int ret;
    ret = mcp23017_write_reg(dev, MCP23017_IODIRA, 0x00);
    if (ret != 0) return ret;
    ret = mcp23017_write_reg(dev, MCP23017_IODIRB, 0x00);
    if (ret != 0) return ret;
    return 0;
}

int mcp23017_read_porta(struct mcp23017_dev *dev, uint8_t *porta_byte)
{
    return mcp23017_read_reg(dev, MCP23017_GPIOA, porta_byte);
}

int mcp23017_read_portb(struct mcp23017_dev *dev, uint8_t *portb_byte)
{
    return mcp23017_read_reg(dev, MCP23017_GPIOB, portb_byte);
}
