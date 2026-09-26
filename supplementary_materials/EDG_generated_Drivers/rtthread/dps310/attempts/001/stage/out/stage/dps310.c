#include "dps310.h"
#include <stdint.h>
#include <string.h>

#define DPS310_I2C_ADDR 0x77

#define PRS_B2 0x00
#define PRS_B1 0x01
#define PRS_B0 0x02
#define TMP_B2 0x03
#define TMP_B1 0x04
#define TMP_B0 0x05
#define PRS_CFG 0x06
#define TMP_CFG 0x07
#define MEAS_CFG 0x08
#define CFG_REG 0x09
#define INT_STS 0x0A
#define FIFO_STS 0x0B
#define RESET 0x0C
#define ID 0x0D
#define COEF_SRCE 0x28

static int i2c_write_then_read(struct rt_i2c_bus_device *bus, uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    struct rt_i2c_msg msgs[2];

    msgs[0].addr = addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf = &reg;
    msgs[0].len = 1;

    msgs[1].addr = addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf = buf;
    msgs[1].len = len;

    if (rt_i2c_transfer(bus, msgs, 2) != 2)
        return -1;
    return 0;
}

static int i2c_write(struct rt_i2c_bus_device *bus, uint8_t addr, uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = {reg, data};
    struct rt_i2c_msg msg;

    msg.addr = addr;
    msg.flags = RT_I2C_WR;
    msg.buf = buf;
    msg.len = 2;

    if (rt_i2c_transfer(bus, &msg, 1) != 1)
        return -1;
    return 0;
}

static int32_t combine_24bit_signed(uint8_t msb, uint8_t mid, uint8_t lsb)
{
    int32_t val = ((int32_t)msb << 16) | ((int32_t)mid << 8) | (int32_t)lsb;
    if (val & 0x800000)
        val |= 0xFF000000;
    return val;
}

int dps310_init(struct dps310_device *dev, struct rt_i2c_bus_device *bus)
{
    uint8_t id;
    dev->bus = bus;
    dev->i2c_addr = DPS310_I2C_ADDR;

    /* Soft reset */
    if (i2c_write(bus, dev->i2c_addr, RESET, 0x89) != 0)
        return -1;
    rt_thread_mdelay(12); /* wait for sensor ready max 12ms */

    /* Read ID register */
    if (i2c_write_then_read(bus, dev->i2c_addr, ID, &id, 1) != 0)
        return -1;

    /* Configure CFG_REG: disable FIFO, no interrupts */
    if (i2c_write(bus, dev->i2c_addr, CFG_REG, 0x00) != 0)
        return -1;

    /* Wait for coefficients ready (max 40ms) */
    rt_thread_mdelay(40);

    return 0;
}

int dps310_read_pressure(struct dps310_device *dev, int32_t *pressure_raw)
{
    uint8_t buf[3];
    if (i2c_write_then_read(dev->bus, dev->i2c_addr, PRS_B2, buf, 3) != 0)
        return -1;
    *pressure_raw = combine_24bit_signed(buf[0], buf[1], buf[2]);
    return 0;
}

int dps310_read_temperature(struct dps310_device *dev, int32_t *temperature_mdegc)
{
    uint8_t buf[3];
    if (i2c_write_then_read(dev->bus, dev->i2c_addr, TMP_B2, buf, 3) != 0)
        return -1;
    int32_t raw_temp = combine_24bit_signed(buf[0], buf[1], buf[2]);
    /* According to Section B3, temperature output is raw count (no conversion) */
    *temperature_mdegc = raw_temp;
    return 0;
}