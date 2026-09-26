#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>
#include <stdint.h>
#include <errno.h>

#include <zephyr/sys/byteorder.h>
#define TMP105_I2C_ADDR 0x48

int tmp105_init(const struct device *dev)
{
    (void)dev;
    return 0;
}

int tmp105_read_temperature(const struct device *dev, int32_t *raw)
{
    struct i2c_dt_spec spec = {
        .bus = dev,
        .addr = TMP105_I2C_ADDR
    };
    uint8_t cmd = 0x00;
    uint8_t buf[2];
    int ret;

    k_sleep(K_MSEC(220));

    ret = i2c_write_dt(&spec, &cmd, 1);
    if (ret != 0) {
        return -EIO;
    }

    ret = i2c_read_dt(&spec, buf, 2);
    if (ret != 0) {
        return -EIO;
    }

    uint16_t raw16 = ((uint16_t)buf[0] << 8) | buf[1];
    int16_t temp12 = (int16_t)(raw16 >> 4);
    if (temp12 & 0x0800) {
        temp12 |= 0xF000;
    }
    *raw = ((int32_t)temp12 * 125) / 2;
    return 0;
}
