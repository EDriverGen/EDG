#include "pca9685.h"
#include <stddef.h>

#include <hal/hal_i2c.h>
#define PCA9685_I2C_ADDR 0x40
#define PCA9685_LED0_OFF_L 0x08
#define PCA9685_MODE1 0x00
#define PCA9685_MODE1_SLEEP (1 << 4)

int pca9685_init(struct pca9685_dev *dev, void *bus_handle) {
    (void)bus_handle;
    dev->i2c_num = 0;
    dev->i2c_addr = PCA9685_I2C_ADDR;
    return 0;
}

static int pca9685_write_then_read(struct pca9685_dev *dev, uint8_t reg, uint8_t *buf, uint16_t len) {
    struct hal_i2c_master_data data;
    data.address = dev->i2c_addr;
    data.len = 1;
    data.buffer = &reg;
    int rc = hal_i2c_master_write(dev->i2c_num, &data, OS_TIME_FOREVER, 1);
    if (rc != 0) return rc;
    data.len = len;
    data.buffer = buf;
    return hal_i2c_master_read(dev->i2c_num, &data, OS_TIME_FOREVER, 1);
}

int pca9685_read_pwm_channel(struct pca9685_dev *dev, uint8_t channel, uint16_t *out) {
    uint8_t reg = PCA9685_LED0_OFF_L + (channel * 4);
    uint8_t buf[4];
    struct hal_i2c_master_data data;
    data.address = dev->i2c_addr;
    data.len = 1;
    data.buffer = &reg;
    int rc = hal_i2c_master_write(dev->i2c_num, &data, OS_TIME_FOREVER, 1);
    if (rc != 0) return rc;
    data.len = 4;
    data.buffer = buf;
    rc = hal_i2c_master_read(dev->i2c_num, &data, OS_TIME_FOREVER, 1);
    if (rc != 0) return rc;
    uint8_t off_l = buf[0];
    uint8_t off_h = buf[1];
    *out = ((uint16_t)(off_h & 0x0F) * 256) + off_l;
    return 0;
}
