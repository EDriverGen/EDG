#include "adxl345.h"
#include "spi_if.h"
#include "hdf_base.h"
#include <string.h>

#define ADXL345_READ_CMD(reg) (0x80 | (reg))
#define ADXL345_MB_CMD(reg) (0xC0 | (reg))

int adxl345_init(struct adxl345_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    struct SpiMsg msg;
    int32_t ret;

    // Write 0x2D, 0x00 to POWER_CTL (standby)
    uint8_t standby_cmd[2] = {0x2D, 0x00};
    (void)memset(&msg, 0, sizeof(msg));
    msg.wbuf = standby_cmd;
    msg.rbuf = NULL;
    msg.len = 2;
    msg.speed = 5000000;
    msg.delayUs = 0;
    msg.csChange = 0;
    msg.keepCs = 0;
    ret = SpiTransfer(dev->bus_handle, &msg, 1);
    if (ret != 0) return -1;

    // Write 0x2D, 0x08 to POWER_CTL (measurement mode)
    uint8_t measure_cmd[2] = {0x2D, 0x08};
    (void)memset(&msg, 0, sizeof(msg));
    msg.wbuf = measure_cmd;
    msg.rbuf = NULL;
    msg.len = 2;
    msg.speed = 5000000;
    msg.delayUs = 0;
    msg.csChange = 0;
    msg.keepCs = 0;
    ret = SpiTransfer(dev->bus_handle, &msg, 1);
    if (ret != 0) return -1;

    return 0;
}

int adxl345_read_xyz(struct adxl345_dev *dev, int16_t *ax, int16_t *ay, int16_t *az)
{
    uint8_t cmd = ADXL345_MB_CMD(0x32); // 0xF2
    uint8_t rxbuf[6];
    struct SpiMsg msg;
    int32_t ret;

    (void)memset(&msg, 0, sizeof(msg));
    msg.wbuf = &cmd;
    msg.rbuf = rxbuf;
    msg.len = 7; // 1 command + 6 data
    msg.speed = 5000000;
    msg.delayUs = 0;
    msg.csChange = 0;
    msg.keepCs = 0;
    ret = SpiTransfer(dev->bus_handle, &msg, 1);
    if (ret != 0) return -1;

    // rxbuf[0] is echo of command, data starts at rxbuf[1]
    uint8_t *data = rxbuf + 1;
    int16_t x = (int16_t)(data[0] | (data[1] << 8));
    int16_t y = (int16_t)(data[2] | (data[3] << 8));
    int16_t z = (int16_t)(data[4] | (data[5] << 8));

    *ax = x;
    *ay = y;
    *az = z;

    return 0;
}