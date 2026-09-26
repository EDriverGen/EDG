#include "mcp23017.h"
#include <stddef.h>

#include <hal/hal_i2c.h>
#define MCP23017_I2C_ADDR 0x20
#define MCP23017_IODIRA 0x00
#define MCP23017_IODIRB 0x01
#define MCP23017_GPIOA 0x12
#define MCP23017_GPIOB 0x13

static int mcp23017_write_reg(struct mcp23017_dev *dev, uint8_t reg, uint8_t data) {
    struct hal_i2c_master_data pdata;
    uint8_t buf[2] = {reg, data};
    pdata.address = dev->addr;
    pdata.buffer = buf;
    pdata.len = 2;
    return hal_i2c_master_write(dev->i2c_num, &pdata, OS_TIMEOUT_NEVER, 1);
}

static int mcp23017_read_reg(struct mcp23017_dev *dev, uint8_t reg, uint8_t *data) {
    struct hal_i2c_master_data pdata;
    uint8_t cmd = reg;
    int rc;
    pdata.address = dev->addr;
    pdata.buffer = &cmd;
    pdata.len = 1;
    rc = hal_i2c_master_write(dev->i2c_num, &pdata, OS_TIMEOUT_NEVER, 0);
    if (rc != 0) return rc;
    pdata.buffer = data;
    pdata.len = 1;
    return hal_i2c_master_read(dev->i2c_num, &pdata, OS_TIMEOUT_NEVER, 1);
}

int mcp23017_init(struct mcp23017_dev *dev, void *bus_handle) {
    dev->i2c_num = (uint8_t)(uintptr_t)bus_handle;
    dev->addr = MCP23017_I2C_ADDR;
    int rc;
    rc = mcp23017_write_reg(dev, MCP23017_IODIRA, 0x00);
    if (rc != 0) return rc;
    rc = mcp23017_write_reg(dev, MCP23017_IODIRB, 0x00);
    return rc;
}

int mcp23017_read_porta(struct mcp23017_dev *dev, uint8_t *porta_byte) {
    return mcp23017_read_reg(dev, MCP23017_GPIOA, porta_byte);
}

int mcp23017_read_portb(struct mcp23017_dev *dev, uint8_t *portb_byte) {
    return mcp23017_read_reg(dev, MCP23017_GPIOB, portb_byte);
}
