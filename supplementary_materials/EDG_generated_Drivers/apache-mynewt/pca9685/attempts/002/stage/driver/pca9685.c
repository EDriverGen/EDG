#include "pca9685.h"
#include <stddef.h>

#include <hal/hal_i2c.h>
#define PCA9685_I2C_ADDR 0x40
#define PCA9685_LED0_OFF_L 0x08

int pca9685_init(struct pca9685_dev *dev, void *bus_handle) {
    (void)bus_handle;
    dev->i2c_num = 0;
    dev->i2c_addr = PCA9685_I2C_ADDR;
    return 0;
}

int pca9685_read_pwm_channel(struct pca9685_dev *dev, uint8_t channel, uint16_t *out) {
    uint8_t reg = PCA9685_LED0_OFF_L + (channel * 4);
    uint8_t write_buf[1] = {reg};
    struct hal_i2c_master_data write_data = {
        .address = (dev->i2c_addr << 1) | 0x00,
        .len = 1,
        .buffer = write_buf
    };
    int rc = hal_i2c_master_write(dev->i2c_num, &write_data, OS_TIMEOUT_NEVER, 1);
    if (rc != 0) {
        return -1;
    }
    uint8_t read_buf[4];
    struct hal_i2c_master_data read_data = {
        .address = (dev->i2c_addr << 1) | 0x01,
        .len = 4,
        .buffer = read_buf
    };
    rc = hal_i2c_master_read(dev->i2c_num, &read_data, OS_TIMEOUT_NEVER, 1);
    if (rc != 0) {
        return -1;
    }
    uint8_t off_l = read_buf[0];
    uint8_t off_h = read_buf[1];
    *out = (uint16_t)((off_h & 0x0F) * 256 + off_l);
    return 0;
}
