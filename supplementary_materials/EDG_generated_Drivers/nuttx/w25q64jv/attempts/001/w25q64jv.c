#include "w25q64jv.h"
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>
#include "arch.h"

#include <nuttx/spi/spi.h>
#include "nuttx.h"
#define CMD_READ_DATA      0x03
#define CMD_PAGE_PROGRAM   0x02
#define CMD_WRITE_ENABLE   0x06
#define CMD_JEDEC_ID       0x9F

#define PAGE_SIZE          256

static int spi_write_then_read(struct spi_dev_s *spi, const uint8_t *txbuf, size_t txlen, uint8_t *rxbuf, size_t rxlen)
{
    int ret;
    SPI_SELECT(spi, 0, true);
    ret = SPI_SEND(spi, txbuf[0]);
    if (ret < 0) {
        SPI_SELECT(spi, 0, false);
        return ret;
    }
    for (size_t i = 1; i < txlen; i++) {
        ret = SPI_SEND(spi, txbuf[i]);
        if (ret < 0) {
            SPI_SELECT(spi, 0, false);
            return ret;
        }
    }
    if (rxbuf != NULL && rxlen > 0) {
        for (size_t i = 0; i < rxlen; i++) {
            ret = SPI_SEND(spi, 0xFF);
            if (ret < 0) {
                SPI_SELECT(spi, 0, false);
                return ret;
            }
            rxbuf[i] = (uint8_t)ret;
        }
    }
    SPI_SELECT(spi, 0, false);
    return OK;
}

int w25q64jv_init(struct w25q64jv_dev_s *dev, struct spi_dev_s *spi)
{
    dev->spi = spi;
    uint8_t txbuf[1] = {CMD_JEDEC_ID};
    uint8_t rxbuf[3];
    int ret = spi_write_then_read(spi, txbuf, 1, rxbuf, 3);
    if (ret < 0) {
        return ret;
    }
    return OK;
}

int w25q64jv_read(struct w25q64jv_dev_s *dev, uint32_t addr, uint8_t *buf, size_t len)
{
    struct spi_dev_s *spi = dev->spi;
    if (spi == NULL) {
        return -ENODEV;
    }
    uint8_t txbuf[4];
    txbuf[0] = CMD_READ_DATA;
    txbuf[1] = (uint8_t)(addr >> 16);
    txbuf[2] = (uint8_t)(addr >> 8);
    txbuf[3] = (uint8_t)(addr);
    int ret = spi_write_then_read(spi, txbuf, 4, buf, len);
    if (ret < 0) {
        return ret;
    }
    return 0;
}

int w25q64jv_write(struct w25q64jv_dev_s *dev, uint32_t addr, const uint8_t *buf, size_t len)
{
    struct spi_dev_s *spi = dev->spi;
    if (spi == NULL) {
        return -ENODEV;
    }
    if (len > PAGE_SIZE) {
        return -EINVAL;
    }
    int ret;
    uint8_t txbuf[1] = {CMD_WRITE_ENABLE};
    ret = spi_write_then_read(spi, txbuf, 1, NULL, 0);
    if (ret < 0) {
        return ret;
    }
    uint8_t cmd_addr[4 + PAGE_SIZE];
    cmd_addr[0] = CMD_PAGE_PROGRAM;
    cmd_addr[1] = (uint8_t)(addr >> 16);
    cmd_addr[2] = (uint8_t)(addr >> 8);
    cmd_addr[3] = (uint8_t)(addr);
    memcpy(&cmd_addr[4], buf, len);
    ret = spi_write_then_read(spi, cmd_addr, 4 + len, NULL, 0);
    if (ret < 0) {
        return ret;
    }
    return 0;
}
