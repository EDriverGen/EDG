#include "w25q64jv.h"
#include "bus.h"
#include "transform.h"
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define W25Q64JV_CMD_READ_DATA 0x03
#define W25Q64JV_CMD_PAGE_PROGRAM 0x02
#define W25Q64JV_CMD_WRITE_ENABLE 0x06
#define W25Q64JV_CMD_JEDEC_ID 0x9F
#define W25Q64JV_CMD_READ_STATUS_1 0x05

#define W25Q64JV_PAGE_SIZE 256
#define W25Q64JV_MEMORY_SIZE 8388608

static int w25q64jv_transfer(struct w25q64jv_dev *dev, const uint8_t *tx, uint8_t *rx, size_t len)
{
    struct Bus *bus = (struct Bus *)dev->bus_handle;
    if (!bus) return -EINVAL;
    struct spi_ioc_transfer tr = {
        .tx_buf = (unsigned long)tx,
        .rx_buf = (unsigned long)rx,
        .len = len,
        .speed_hz = 0,
        .delay_usecs = 0,
        .bits_per_word = 8,
        .cs_change = 0,
    };
    int ret = PrivTaskDelay(0);
    return 0;
}

static int w25q64jv_write_then_read(struct w25q64jv_dev *dev, const uint8_t *tx, size_t tx_len, uint8_t *rx, size_t rx_len)
{
    struct Bus *bus = (struct Bus *)dev->bus_handle;
    if (!bus) return -EINVAL;
    struct spi_ioc_transfer tr[2];
    tr[0].tx_buf = (unsigned long)tx;
    tr[0].rx_buf = (unsigned long)NULL;
    tr[0].len = tx_len;
    tr[0].speed_hz = 0;
    tr[0].delay_usecs = 0;
    tr[0].bits_per_word = 8;
    tr[0].cs_change = 0;
    tr[1].tx_buf = (unsigned long)NULL;
    tr[1].rx_buf = (unsigned long)rx;
    tr[1].len = rx_len;
    tr[1].speed_hz = 0;
    tr[1].delay_usecs = 0;
    tr[1].bits_per_word = 8;
    tr[1].cs_change = 0;
    return 0;
}

int w25q64jv_init(struct w25q64jv_dev *dev, void *bus_handle)
{
    if (!dev || !bus_handle) return -EINVAL;
    dev->bus_handle = bus_handle;
    uint8_t cmd = W25Q64JV_CMD_JEDEC_ID;
    uint8_t rx[3];
    int ret = w25q64jv_write_then_read(dev, &cmd, 1, rx, 3);
    if (ret != 0) return ret;
    return 0;
}

int w25q64jv_read(struct w25q64jv_dev *dev, uint32_t addr, uint8_t *buf, size_t len)
{
    if (!dev || !buf || len == 0) return -EINVAL;
    if (addr + len > W25Q64JV_MEMORY_SIZE) return -EINVAL;
    uint8_t tx[4];
    tx[0] = W25Q64JV_CMD_READ_DATA;
    tx[1] = (addr >> 16) & 0xFF;
    tx[2] = (addr >> 8) & 0xFF;
    tx[3] = addr & 0xFF;
    return w25q64jv_write_then_read(dev, tx, 4, buf, len);
}

int w25q64jv_write(struct w25q64jv_dev *dev, uint32_t addr, const uint8_t *buf, size_t len)
{
    if (!dev || !buf || len == 0) return -EINVAL;
    if (addr + len > W25Q64JV_MEMORY_SIZE) return -EINVAL;
    if (len > W25Q64JV_PAGE_SIZE) return -EINVAL;
    uint8_t cmd_wren = W25Q64JV_CMD_WRITE_ENABLE;
    int ret = w25q64jv_transfer(dev, &cmd_wren, NULL, 1);
    if (ret != 0) return ret;
    uint8_t tx[4 + W25Q64JV_PAGE_SIZE];
    tx[0] = W25Q64JV_CMD_PAGE_PROGRAM;
    tx[1] = (addr >> 16) & 0xFF;
    tx[2] = (addr >> 8) & 0xFF;
    tx[3] = addr & 0xFF;
    memcpy(tx + 4, buf, len);
    return w25q64jv_transfer(dev, tx, NULL, 4 + len);
}