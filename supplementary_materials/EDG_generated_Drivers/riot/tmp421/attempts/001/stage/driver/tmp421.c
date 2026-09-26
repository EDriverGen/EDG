#include "tmp421.h"
#include <stdint.h>
#include <stddef.h>
#include "periph/i2c.h"
#include "xtimer.h"

#include "riot.h"
#define TMP421_ADDR 0x2A
#define REG_STATUS 0x08
#define REG_LOCAL_HIGH 0x00
#define REG_LOCAL_LOW 0x10
#define REG_REMOTE_HIGH 0x01
#define REG_REMOTE_LOW 0x11
#define REG_MANUFACTURER_ID 0xFE
#define REG_DEVICE_ID 0xFF
#define BUSY_BIT 0x80

static int poll_busy(struct tmp421_device *dev) {
    uint8_t status;
    int ret;
    for (int i = 0; i < 10; i++) {
        ret = i2c_read_regs(dev->bus, dev->addr, REG_STATUS, &status, 1, 0);
        if (ret != 0) return -1;
        if (!(status & BUSY_BIT)) return 0;
        xtimer_msleep(15);
    }
    return -1;
}

static int read_temp_raw(struct tmp421_device *dev, uint8_t high_reg, uint8_t low_reg, int32_t *result) {
    uint8_t high_byte, low_byte;
    int ret;
    ret = i2c_read_regs(dev->bus, dev->addr, high_reg, &high_byte, 1, 0);
    if (ret != 0) return -1;
    ret = i2c_read_regs(dev->bus, dev->addr, low_reg, &low_byte, 1, 0);
    if (ret != 0) return -1;
    int16_t high_signed = (int16_t)(int8_t)high_byte;
    int32_t combined = ((int32_t)high_signed << 4) + (low_byte >> 4);
    *result = combined * 625 / 10;
    return 0;
}

int tmp421_init(struct tmp421_device *dev, i2c_t bus, uint16_t addr) {
    dev->bus = bus;
    dev->addr = addr;
    uint8_t val;
    int ret;
    ret = i2c_read_regs(bus, addr, REG_MANUFACTURER_ID, &val, 1, 0);
    if (ret != 0 || val != 0x55) return -1;
    ret = i2c_read_regs(bus, addr, REG_DEVICE_ID, &val, 1, 0);
    if (ret != 0 || val != 0x21) return -1;
    return 0;
}

int tmp421_read_local(struct tmp421_device *dev, int32_t *temp_local_val) {
    if (poll_busy(dev) != 0) return -1;
    return read_temp_raw(dev, REG_LOCAL_HIGH, REG_LOCAL_LOW, temp_local_val);
}

int tmp421_read_remote(struct tmp421_device *dev, int32_t *temp_remote_val) {
    if (poll_busy(dev) != 0) return -1;
    return read_temp_raw(dev, REG_REMOTE_HIGH, REG_REMOTE_LOW, temp_remote_val);
}
