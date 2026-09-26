#include "max31855.h"
#include "osal_time.h"
#include <stdint.h>
#include <string.h>

#define MAX31855_CS_INACTIVE_TIME_US 1

static int32_t max31855_spi_read(struct max31855_device *dev, uint8_t *buf, uint32_t len)
{
    struct SpiMsg msg;
    memset(&msg, 0, sizeof(msg));
    msg.wbuf = NULL;
    msg.rbuf = buf;
    msg.len = len;
    msg.speed = 1000000;
    msg.delayUs = 0;
    msg.csChange = 0;
    msg.keepCs = 0;
    int32_t ret = SpiCntlrTransfer(dev->spi_handle, &msg, 1);
    return ret;
}

int32_t max31855_init(struct max31855_device *dev, DevHandle bus_handle)
{
    if (dev == NULL || bus_handle == NULL) {
        return -1;
    }
    dev->spi_handle = bus_handle;
    OsalMSleep(200);
    return 0;
}

int32_t max31855_read_temperatures(struct max31855_device *dev, int32_t *tc, int32_t *local)
{
    if (dev == NULL || tc == NULL || local == NULL) {
        return -1;
    }
    uint8_t buf[4];
    memset(buf, 0, sizeof(buf));
    int32_t ret = max31855_spi_read(dev, buf, 4);
    if (ret != 0) {
        return ret;
    }
    uint32_t raw = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | buf[3];
    if (raw & 0x10000) {
        return 1;
    }
    uint16_t raw_tc = (raw >> 18) & 0x3FFF;
    uint16_t raw_local = (raw >> 4) & 0xFFF;
    int32_t tc_val = (((raw_tc >> 13) & 1) * (-8192) + ((raw_tc & 0x1FFF) ^ ((raw_tc >> 13) & 1) * 0x2000)) * 250 / 1000;
    int32_t local_val = (((raw_local >> 11) & 1) * (-2048) + ((raw_local & 0x7FF) ^ ((raw_local >> 11) & 1) * 0x800)) * 625 / 10000;
    *tc = tc_val;
    *local = local_val;
    return 0;
}