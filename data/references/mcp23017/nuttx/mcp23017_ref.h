#ifndef __MCP23017_REF_H
#define __MCP23017_REF_H

#include <nuttx/config.h>
#include <nuttx/i2c/i2c_master.h>
#include <stdint.h>

#define MCP23017_I2C_ADDR  0x20
#define MCP23017_I2C_FREQ  100000
#define MCP23017_REG_GPIOA 0x12

struct mcp23017_device {
    FAR struct i2c_master_s *i2c;
    struct i2c_config_s config;
};

int mcp23017_init(FAR struct mcp23017_device *dev,
                  FAR struct i2c_master_s *i2c, uint8_t addr);
int mcp23017_read_ports(FAR struct mcp23017_device *dev,
                         uint8_t *porta, uint8_t *portb);

int mcp23017_read_gpa0(FAR struct mcp23017_device *dev);
int mcp23017_read_gpa1(FAR struct mcp23017_device *dev);
int mcp23017_read_gpa2(FAR struct mcp23017_device *dev);
int mcp23017_read_gpa3(FAR struct mcp23017_device *dev);
int mcp23017_read_gpa4(FAR struct mcp23017_device *dev);
int mcp23017_read_gpa5(FAR struct mcp23017_device *dev);
int mcp23017_read_gpa6(FAR struct mcp23017_device *dev);
int mcp23017_read_gpa7(FAR struct mcp23017_device *dev);
int mcp23017_read_gpb0(FAR struct mcp23017_device *dev);
int mcp23017_read_gpb1(FAR struct mcp23017_device *dev);
int mcp23017_read_gpb2(FAR struct mcp23017_device *dev);
int mcp23017_read_gpb3(FAR struct mcp23017_device *dev);
int mcp23017_read_gpb4(FAR struct mcp23017_device *dev);
int mcp23017_read_gpb5(FAR struct mcp23017_device *dev);
int mcp23017_read_gpb6(FAR struct mcp23017_device *dev);
int mcp23017_read_gpb7(FAR struct mcp23017_device *dev);

#endif
