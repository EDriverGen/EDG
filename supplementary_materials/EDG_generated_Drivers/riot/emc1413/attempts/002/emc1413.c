#include "emc1413.h"
#include <stddef.h>
#include <stdint.h>

#include "riot.h"
#define EMC1413_ADDR 0x4C
#define REG_MANUFACTURER_ID 0xFE
#define REG_PRODUCT_ID 0xFD
#define REG_CONFIG 0x03
#define REG_CONV_RATE 0x04
#define REG_INT_HIGH 0x00
#define REG_INT_LOW 0x29
#define REG_EXT1_HIGH 0x01
#define REG_EXT1_LOW 0x10
#define REG_EXT2_HIGH 0x23
#define REG_EXT2_LOW 0x24

static int read_reg(emc1413_t *dev, uint16_t reg, uint8_t *val) {
    i2c_acquire(dev->bus);
    int ret = i2c_read_regs(dev->bus, dev->addr, reg, val, 1, 0);
    i2c_release(dev->bus);
    return ret;
}

static int write_reg(emc1413_t *dev, uint16_t reg, uint8_t val) {
    uint8_t buf[2] = { (uint8_t)(reg & 0xFF), val };
    i2c_acquire(dev->bus);
    int ret = i2c_write_bytes(dev->bus, dev->addr, buf, 2, 0);
    i2c_release(dev->bus);
    return ret;
}

int emc1413_init(emc1413_t *dev, i2c_t bus) {
    dev->bus = bus;
    dev->addr = EMC1413_ADDR;

    ztimer_sleep(ZTIMER_MSEC, 15);

    uint8_t id;
    if (read_reg(dev, REG_MANUFACTURER_ID, &id) < 0 || id != 0x5D) return -1;
    if (read_reg(dev, REG_PRODUCT_ID, &id) < 0 || id != 0x21) return -1;

    if (write_reg(dev, REG_CONFIG, 0x00) < 0) return -1;
    if (write_reg(dev, REG_CONV_RATE, 0x06) < 0) return -1;

    return 0;
}

static int read_temperature(emc1413_t *dev, uint16_t reg_high, uint16_t reg_low, int32_t *out) {
    uint8_t high, low;
    if (read_reg(dev, reg_high, &high) < 0) return -1;
    if (read_reg(dev, reg_low, &low) < 0) return -1;
    int32_t temp = ((int32_t)high * 1000) + ((((low >> 5) & 0x07) * 125));
    *out = temp;
    return 0;
}

int emc1413_read_internal_temperature(emc1413_t *dev, int32_t *temp_local_val) {
    return read_temperature(dev, REG_INT_HIGH, REG_INT_LOW, temp_local_val);
}

int emc1413_read_external_diode_1_temperature(emc1413_t *dev, int32_t *temp_ext1_val) {
    return read_temperature(dev, REG_EXT1_HIGH, REG_EXT1_LOW, temp_ext1_val);
}

int emc1413_read_external_diode_2_temperature(emc1413_t *dev, int32_t *temp_ext2_val) {
    return read_temperature(dev, REG_EXT2_HIGH, REG_EXT2_LOW, temp_ext2_val);
}
