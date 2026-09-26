#include "w25q64jv.h"
#include "bus.h"
#include "transform.h"
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define W25Q64JV_CMD_READ_DATA  0x03
#define W25Q64JV_CMD_PAGE_PROGRAM 0x02
#define W25Q64JV_CMD_WRITE_ENABLE 0x06
#define W25Q64JV_CMD_JEDEC_ID   0x9F

static int spi_write_then_read(struct w25q64jv_dev *dev, const uint8_t *tx, size_t tx_len, uint8_t *rx, size_t rx_len)
{
    struct Bus *bus = (struct Bus *)dev->bus_handle;
    struct spi_ioc_transfer tr[2];
    int ret;

    memset(tr, 0, sizeof(tr));
    tr[0].tx_buf = (const void *)(uintptr_t)tx;
    tr[0].rx_buf = NULL;
    tr[0].len = tx_len;
    tr[0].speed_hz = 0;
    tr[0].delay_usecs = 0;
    tr[0].bits_per_word = 8;
    tr[0].cs_change = 0;

    tr[1].tx_buf = NULL;
    tr[1].rx_buf = (void *)(uintptr_t)rx;
    tr[1].len = rx_len;
    tr[1].speed_hz = 0;
    tr[1].delay_usecs = 0;
    tr[1].bits_per_word = 8;
    tr[1].cs_change = 1;

    ret = PrivTaskDelay(0);
    if (ret < 0) return ret;

    return 0;
}

static int spi_write(struct w25q64jv_dev *dev, const uint8_t *data, size_t len)
{
    return spi_write_then_read(dev, data, len, NULL, 0);
}

static int spi_read(struct w25q64jv_dev *dev, uint8_t *rx, size_t len)
{
    return spi_write_then_read(dev, NULL, 0, rx, len);
}

int w25q64jv_init(struct w25q64jv_dev *dev, void *bus_handle)
{
    uint8_t cmd[1];
    uint8_t id[3];
    int ret;

    dev->bus_handle = bus_handle;

    cmd[0] = W25Q64JV_CMD_JEDEC_ID;
    ret = spi_write(dev, cmd, 1);
    if (ret < 0) return ret;

    ret = spi_read(dev, id, 3);
    if (ret < 0) return ret;

    return 0;
}

int w25q64jv_read(struct w25q64jv_dev *dev, uint32_t addr, uint8_t *buf, size_t len)
{
    uint8_t cmd[4];
    int ret;

    cmd[0] = W25Q64JV_CMD_READ_DATA;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;

    ret = spi_write(dev, cmd, 4);
    if (ret < 0) return ret;

    ret = spi_read(dev, buf, len);
    if (ret < 0) return ret;

    return 0;
}

int w25q64jv_write(struct w25q64jv_dev *dev, uint32_t addr, const uint8_t *buf, size_t len)
{
    uint8_t cmd[4];
    uint8_t enable_cmd[1];
    int ret;

    enable_cmd[0] = W25Q64JV_CMD_WRITE_ENABLE;
    ret = spi_write(dev, enable_cmd, 1);
    if (ret < 0) return ret;

    cmd[0] = W25Q64JV_CMD_PAGE_PROGRAM;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;

    ret = spi_write(dev, cmd, 4);
    if (ret < 0) return ret;

    ret = spi_write(dev, buf, len);
    if (ret < 0) return ret;

    return 0;
}