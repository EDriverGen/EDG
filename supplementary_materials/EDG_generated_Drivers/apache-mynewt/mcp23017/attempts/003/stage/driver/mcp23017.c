#include "mcp23017.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include <hal/hal_i2c.h>
#include <os/os_time.h>
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
    uint8_t reg_buf = reg;
    pdata.address = dev->addr;
    pdata.buffer = &reg_buf;
    pdata.len = 1;
    int ret = hal_i2c_master_write(dev->i2c_num, &pdata, OS_TIMEOUT_NEVER, 0);
    if (ret != 0) return ret;
    pdata.buffer = data;
    pdata.len = 1;
    return hal_i2c_master_read(dev->i2c_num, &pdata, OS_TIMEOUT_NEVER, 1);
}

int mcp23017_init(struct mcp23017_dev *dev, void *bus_handle) {
    (void)bus_handle;
    dev->i2c_num = 0;
    dev->addr = MCP23017_I2C_ADDR;
    os_time_delay(OS_TICKS_PER_SEC / 100);
    int ret = mcp23017_write_reg(dev, MCP23017_IODIRA, 0x00);
    if (ret != 0) return ret;
    ret = mcp23017_write_reg(dev, MCP23017_IODIRB, 0x00);
    return ret;
}

int mcp23017_read_porta(struct mcp23017_dev *dev, uint8_t *porta_byte) {
    return mcp23017_read_reg(dev, MCP23017_GPIOA, porta_byte);
}

int mcp23017_read_portb(struct mcp23017_dev *dev, uint8_t *portb_byte) {
    return mcp23017_read_reg(dev, MCP23017_GPIOB, portb_byte);
}
