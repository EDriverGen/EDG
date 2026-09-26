/* W25Q64JV for Zephyr - simplified for stub environment. */
#include "w25q64jv_ref.h"
static int w25q64jv_xfer(struct w25q64jv_device *dev, const uint8_t *tx, uint8_t *rx, size_t len) {
    struct spi_buf txb = {.buf=(void*)tx, .len=len};
    struct spi_buf rxb = {.buf=rx, .len=rx?len:0};
    struct spi_buf_set txs = {.buffers=&txb, .count=1};
    struct spi_buf_set rxs = {.buffers=&rxb, .count=rx?1:0};
    return spi_transceive_dt((struct spi_dt_spec*)&dev->bus, &txs, &rxs);
}
static int w25q64jv_wait_ready(struct w25q64jv_device *dev) {
    uint8_t cmd = W25Q64_CMD_READ_SR1, sr1;
    for (int i = 0; i < 10000; i++) {
        w25q64jv_xfer(dev, &cmd, &sr1, 1);
        if (!(sr1 & W25Q64_SR1_BUSY)) return 0;
        for (volatile int _x = 0; _x < 100000; _x++);
    }
    return -1;
}
static int w25q64jv_write_enable(struct w25q64jv_device *dev) {
    uint8_t cmd = W25Q64_CMD_WRITE_ENABLE;
    return w25q64jv_xfer(dev, &cmd, NULL, 1) == 0 ? 0 : -1;
}
int w25q64jv_init(struct w25q64jv_device *dev, const struct spi_dt_spec *bus) {
    if (!dev || !bus) return -1;
    memcpy((void*)&dev->bus, bus, sizeof(*bus));
    return 0;
}
int w25q64jv_probe(struct w25q64jv_device *dev) {
    uint8_t cmd = W25Q64_CMD_JEDEC_ID, id[3];
    if (w25q64jv_xfer(dev, &cmd, id, 4) != 0) return -1;
    return (id[1] == W25Q64_JEDEC_MFR_WINBOND) ? 0 : -1;
}
int w25q64jv_read(struct w25q64jv_device *dev, uint32_t addr, uint8_t *buf, uint32_t len) {
    if (!dev || !buf || len == 0) return -1;
    uint8_t tx[4] = {W25Q64_CMD_READ_DATA, (uint8_t)(addr>>16), (uint8_t)(addr>>8), (uint8_t)addr};
    w25q64jv_xfer(dev, tx, NULL, 4);
    return w25q64jv_xfer(dev, NULL, buf, len) == 0 ? 0 : -1;
}
int w25q64jv_write(struct w25q64jv_device *dev, uint32_t addr, const uint8_t *buf, uint32_t len) {
    if (!dev || !buf || len == 0) return -1;
    uint32_t offset = 0;
    while (offset < len) {
        uint32_t pr = W25Q64JV_PAGE_SIZE - ((addr+offset) % W25Q64JV_PAGE_SIZE);
        uint32_t chunk = (len-offset < pr) ? (len-offset) : pr;
        w25q64jv_write_enable(dev);
        uint8_t hdr[4] = {W25Q64_CMD_PAGE_PROGRAM, (uint8_t)((addr+offset)>>16), (uint8_t)((addr+offset)>>8), (uint8_t)(addr+offset)};
        w25q64jv_xfer(dev, hdr, NULL, 4);
        w25q64jv_xfer(dev, (uint8_t*)(buf+offset), NULL, chunk);
        if (w25q64jv_wait_ready(dev) != 0) return -1;
        offset += chunk;
    }
    return 0;
}
int w25q64jv_erase_sector(struct w25q64jv_device *dev, uint32_t addr) {
    w25q64jv_write_enable(dev);
    uint8_t cmd[4] = {W25Q64_CMD_SECTOR_ERASE, (uint8_t)(addr>>16), (uint8_t)(addr>>8), (uint8_t)addr};
    w25q64jv_xfer(dev, cmd, NULL, 4);
    return w25q64jv_wait_ready(dev);
}
