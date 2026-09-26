#include "adxl345.h"
#include "hdf_base.h"
#include <string.h>

#include "spi_if.h"
#define ADXL345_POWER_CTL 0x2D
#define ADXL345_DATAX0    0x32
#define ADXL345_READ_BURST 0xF2

static int spi_write(struct adxl345_dev *dev, uint8_t reg, uint8_t value)
{
    uint8_t tx[2] = {reg, value};
    struct SpiMsg msg;
    memset(&msg, 0, sizeof(msg));
    msg.wbuf = tx;
    msg.rbuf = NULL;
    msg.len = 2;
    msg.speed = 5000000;
    msg.delayUs = 0;
    msg.csChange = 0;
    msg.keepCs = 0;
    int32_t ret = SpiTransfer(dev->spi_handle, &msg, 1);
    return (ret == 0) ? 0 : -1;
}

static int spi_read_burst(struct adxl345_dev *dev, uint8_t cmd, uint8_t *buf, uint32_t len)
{
    uint8_t tx[1] = {cmd};
    uint8_t rx[1 + len];
    struct SpiMsg msg[2];
    memset(msg, 0, sizeof(msg));
    msg[0].wbuf = tx;
    msg[0].rbuf = NULL;
    msg[0].len = 1;
    msg[0].speed = 5000000;
    msg[0].delayUs = 0;
    msg[0].csChange = 0;
    msg[0].keepCs = 1;
    msg[1].wbuf = NULL;
    msg[1].rbuf = rx;
    msg[1].len = len;
    msg[1].speed = 5000000;
    msg[1].delayUs = 0;
    msg[1].csChange = 0;
    msg[1].keepCs = 0;
    int32_t ret = SpiTransfer(dev->spi_handle, msg, 2);
    if (ret != 0) return -1;
    memcpy(buf, rx, len);
    return 0;
}

int adxl345_init(struct adxl345_dev *dev, DevHandle bus_handle)
{
    dev->spi_handle = bus_handle;
    if (dev->spi_handle == NULL) return -1;
    int ret;
    ret = spi_write(dev, ADXL345_POWER_CTL, 0x00);
    if (ret != 0) return -1;
    ret = spi_write(dev, ADXL345_POWER_CTL, 0x08);
    if (ret != 0) return -1;
    return 0;
}

int adxl345_read_xyz(struct adxl345_dev *dev, int16_t *ax, int16_t *ay, int16_t *az)
{
    uint8_t buf[6];
    int ret = spi_read_burst(dev, ADXL345_READ_BURST, buf, 6);
    if (ret != 0) return -1;
    *ax = (int16_t)(buf[0] | (buf[1] << 8));
    *ay = (int16_t)(buf[2] | (buf[3] << 8));
    *az = (int16_t)(buf[4] | (buf[5] << 8));
    return 0;
}
