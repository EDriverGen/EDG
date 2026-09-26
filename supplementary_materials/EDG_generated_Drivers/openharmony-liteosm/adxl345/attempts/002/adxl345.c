#include "adxl345.h"
#include "spi_if.h"
#include "hdf_base.h"
#include <stdint.h>
#include <string.h>

#define ADXL345_REG_POWER_CTL 0x2D
#define ADXL345_REG_DATAX0    0x32
#define ADXL345_READ_CMD      0x80
#define ADXL345_MB_CMD        0x40

int adxl345_init(struct adxl345_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    struct SpiMsg msgs[2];
    int32_t ret;

    // Write 0x2D, 0x00: standby
    uint8_t standby_cmd[2] = { ADXL345_REG_POWER_CTL, 0x00 };
    (void)memset_s(&msgs[0], sizeof(struct SpiMsg), 0, sizeof(struct SpiMsg));
    msgs[0].wbuf = standby_cmd;
    msgs[0].len = 2;
    msgs[0].csChange = 1;
    ret = SpiTransfer(dev->bus_handle, msgs, 1);
    if (ret != 0) {
        return -1;
    }

    // Write 0x2D, 0x08: measurement mode
    uint8_t measure_cmd[2] = { ADXL345_REG_POWER_CTL, 0x08 };
    (void)memset_s(&msgs[0], sizeof(struct SpiMsg), 0, sizeof(struct SpiMsg));
    msgs[0].wbuf = measure_cmd;
    msgs[0].len = 2;
    msgs[0].csChange = 1;
    ret = SpiTransfer(dev->bus_handle, msgs, 1);
    if (ret != 0) {
        return -1;
    }

    return 0;
}

int adxl345_read_xyz(struct adxl345_dev *dev, int16_t *x, int16_t *y, int16_t *z)
{
    if (dev == NULL || dev->bus_handle == NULL || x == NULL || y == NULL || z == NULL) {
        return -1;
    }

    uint8_t cmd = ADXL345_REG_DATAX0 | ADXL345_READ_CMD | ADXL345_MB_CMD; // 0xF2
    uint8_t rx_buf[6];
    struct SpiMsg msg;
    (void)memset_s(&msg, sizeof(struct SpiMsg), 0, sizeof(struct SpiMsg));
    msg.wbuf = &cmd;
    msg.rbuf = rx_buf;
    msg.len = 7; // 1 command + 6 data
    msg.csChange = 1;

    int32_t ret = SpiTransfer(dev->bus_handle, &msg, 1);
    if (ret != 0) {
        return -1;
    }

    // rx_buf[0] is command echo, data starts at rx_buf[1]
    uint8_t *data = &rx_buf[1];
    *x = (int16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));
    *y = (int16_t)((uint16_t)data[2] | ((uint16_t)data[3] << 8));
    *z = (int16_t)((uint16_t)data[4] | ((uint16_t)data[5] << 8));

    return 0;
}