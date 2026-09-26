#include "pcf8574.h"
#include "i2c_msg.h"
#include "hdf_base.h"
#include <stdint.h>

#include "openharmony_liteosm.h"
int pcf8574_init(struct pcf8574_device *dev, DevHandle bus_handle)
{
    if (dev == NULL || bus_handle == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    dev->bus_handle = bus_handle;
    dev->i2c_addr = PCF8574_I2C_ADDR;
    return HDF_SUCCESS;
}

int pcf8574_read_port(struct pcf8574_device *dev, uint8_t *p0, uint8_t *p1, uint8_t *p2, uint8_t *p3, uint8_t *p4, uint8_t *p5, uint8_t *p6, uint8_t *p7)
{
    if (dev == NULL || dev->bus_handle == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    if (p0 == NULL || p1 == NULL || p2 == NULL || p3 == NULL ||
        p4 == NULL || p5 == NULL || p6 == NULL || p7 == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }

    uint8_t buf[1];
    struct I2cMsg msgs[1];
    msgs[0].addr = dev->i2c_addr;
    msgs[0].flags = 0; /* read */
    msgs[0].buf = buf;
    msgs[0].len = 1;

    int32_t ret = I2cTransfer(dev->bus_handle, msgs, 1);
    if (ret != 1) {
        return HDF_FAILURE;
    }

    uint8_t port = buf[0];
    *p0 = (port >> 0) & 1;
    *p1 = (port >> 1) & 1;
    *p2 = (port >> 2) & 1;
    *p3 = (port >> 3) & 1;
    *p4 = (port >> 4) & 1;
    *p5 = (port >> 5) & 1;
    *p6 = (port >> 6) & 1;
    *p7 = (port >> 7) & 1;

    return HDF_SUCCESS;
}
