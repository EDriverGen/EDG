#include "pcf8574.h"
#include <stdint.h>

#include <hal/hal_i2c.h>
#define PCF8574_I2C_ADDR 0x20

int pcf8574_init(struct pcf8574_dev *dev, void *bus_handle) {
    (void)bus_handle;
    dev->i2c_num = 0;
    dev->addr = PCF8574_I2C_ADDR;
    return 0;
}

int pcf8574_read_port(struct pcf8574_dev *dev, uint8_t *p0, uint8_t *p1, uint8_t *p2, uint8_t *p3, uint8_t *p4, uint8_t *p5, uint8_t *p6, uint8_t *p7) {
    uint8_t buf[1];
    struct hal_i2c_master_data pdata;
    int rc;

    pdata.address = dev->addr;
    pdata.len = 1;
    pdata.buffer = buf;

    rc = hal_i2c_master_read(dev->i2c_num, &pdata, 1000, 1);
    if (rc != 0) {
        return rc;
    }

    *p0 = (buf[0] >> 0) & 1;
    *p1 = (buf[0] >> 1) & 1;
    *p2 = (buf[0] >> 2) & 1;
    *p3 = (buf[0] >> 3) & 1;
    *p4 = (buf[0] >> 4) & 1;
    *p5 = (buf[0] >> 5) & 1;
    *p6 = (buf[0] >> 6) & 1;
    *p7 = (buf[0] >> 7) & 1;

    return 0;
}
