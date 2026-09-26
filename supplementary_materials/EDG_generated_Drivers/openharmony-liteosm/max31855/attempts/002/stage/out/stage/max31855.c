#include "max31855.h"
#include "spi_if.h"
#include "hdf_base.h"
#include "osal_time.h"
#include <stdint.h>

#define MAX31855_CS_INACTIVE_TIME_US 1

int32_t max31855_init(struct max31855_device *dev, DevHandle bus_handle)
{
    if (dev == NULL || bus_handle == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }
    dev->spi_handle = bus_handle;
    /* Wait for power-up conversion time: 200 ms minimum */
    OsalMSleep(200);
    return HDF_SUCCESS;
}

int32_t max31855_read_temperatures(struct max31855_device *dev, int32_t *tc, int32_t *local)
{
    if (dev == NULL || tc == NULL || local == NULL) {
        return HDF_ERR_INVALID_PARAM;
    }

    uint8_t rx_buf[4] = {0};
    struct SpiMsg msg;
    msg.wbuf = NULL;
    msg.rbuf = rx_buf;
    msg.len = 4;
    msg.speed = 0;
    msg.delayUs = 0;
    msg.csChange = 0;
    msg.keepCs = 0;

    int32_t ret = SpiTransfer(dev->spi_handle, &msg, 1);
    if (ret != HDF_SUCCESS) {
        return ret;
    }

    /* Build 32-bit frame from received bytes (MSB first) */
    uint32_t frame = ((uint32_t)rx_buf[0] << 24) |
                     ((uint32_t)rx_buf[1] << 16) |
                     ((uint32_t)rx_buf[2] << 8) |
                     ((uint32_t)rx_buf[3]);

    /* Check fault bit D16 */
    if (frame & 0x00010000) {
        return HDF_FAILURE;
    }

    /* Extract thermocouple temperature: bits D[31:18] (14-bit signed) */
    uint16_t raw_tc = (uint16_t)((frame >> 18) & 0x3FFF);
    int32_t tc_signed;
    if (raw_tc & 0x2000) {
        tc_signed = (int32_t)(raw_tc | 0xFFFFC000);
    } else {
        tc_signed = (int32_t)raw_tc;
    }
    /* Convert to milli_degC: tc_signed * 250 / 1000 */
    *tc = (tc_signed * 250) / 1000;

    /* Extract internal temperature: bits D[15:4] (12-bit signed) */
    uint16_t raw_local = (uint16_t)((frame >> 4) & 0xFFF);
    int32_t local_signed;
    if (raw_local & 0x800) {
        local_signed = (int32_t)(raw_local | 0xFFFFF000);
    } else {
        local_signed = (int32_t)raw_local;
    }
    /* Convert to milli_degC: local_signed * 625 / 10000 */
    *local = (local_signed * 625) / 10000;

    return HDF_SUCCESS;
}