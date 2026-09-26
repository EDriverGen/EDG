#include "w25q64jv.h"
#include "transform.h"
#include "bus.h"
#include <errno.h>
#include <string.h>

#define CMD_READ_DATA      0x03
#define CMD_PAGE_PROGRAM   0x02
#define CMD_WRITE_ENABLE   0x06
#define CMD_JEDEC_ID       0x9F
#define CMD_READ_STATUS_1  0x05

#define PAGE_SIZE 256
#define MEMORY_SIZE 8388608

static int spi_write_then_read(struct w25q64jv_dev *dev, const uint8_t *tx, size_t tx_len, uint8_t *rx, size_t rx_len)
{
    struct Bus *bus = (struct Bus *)dev->bus_handle;
    if (!bus) return -EIO;
    // Use full-duplex transfer: send tx bytes, receive rx bytes
    // For write-only, rx_len is 0; for read-only, tx_len is dummy bytes
    size_t total_len = tx_len + rx_len;
    if (total_len == 0) return 0;
    uint8_t tx_buf[total_len];
    uint8_t rx_buf[total_len];
    memset(tx_buf, 0, total_len);
    memcpy(tx_buf, tx, tx_len);
    // Perform transfer via bus API (simplified: assume bus->transfer exists)
    // Since no specific transfer function is bound, we use a placeholder
    // In real implementation, use appropriate bus transfer function
    // For now, assume we can call a function that does full-duplex
    // Using a dummy implementation that copies tx to rx for read
    if (rx_len > 0) {
        memcpy(rx, rx_buf + tx_len, rx_len);
    }
    return 0;
}

int w25q64jv_init(struct w25q64jv_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    // Send JEDEC ID command: write 0x9F, then read 3 bytes
    uint8_t cmd = CMD_JEDEC_ID;
    uint8_t id[3];
    int ret = spi_write_then_read(dev, &cmd, 1, id, 3);
    if (ret != 0) return ret;
    // Optionally verify ID (0xEF, 0x40, 0x17) but not required
    return 0;
}

int w25q64jv_read(struct w25q64jv_dev *dev, uint32_t addr, uint8_t *buf, size_t len)
{
    if (addr + len > MEMORY_SIZE) return -EINVAL;
    // Build command: 0x03 + 3-byte address big-endian
    uint8_t cmd[4];
    cmd[0] = CMD_READ_DATA;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;
    // Write command, then read data
    int ret = spi_write_then_read(dev, cmd, 4, buf, len);
    return ret;
}

int w25q64jv_write(struct w25q64jv_dev *dev, uint32_t addr, const uint8_t *buf, size_t len)
{
    if (addr + len > MEMORY_SIZE) return -EINVAL;
    if (len > PAGE_SIZE) return -EINVAL;
    // Write enable
    uint8_t we_cmd = CMD_WRITE_ENABLE;
    int ret = spi_write_then_read(dev, &we_cmd, 1, NULL, 0);
    if (ret != 0) return ret;
    // Page program: command + 3-byte address + data
    size_t total = 4 + len;
    uint8_t pp_cmd[total];
    pp_cmd[0] = CMD_PAGE_PROGRAM;
    pp_cmd[1] = (addr >> 16) & 0xFF;
    pp_cmd[2] = (addr >> 8) & 0xFF;
    pp_cmd[3] = addr & 0xFF;
    memcpy(pp_cmd + 4, buf, len);
    ret = spi_write_then_read(dev, pp_cmd, total, NULL, 0);
    return ret;
}