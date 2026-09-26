/* W25Q64JV for ChibiOS/HAL. */
#include "w25q64jv_ref.h"
static int w25q64jv_wait_ready(struct w25q64jv_device *dev) {
    uint8_t cmd = W25Q64_CMD_READ_SR1, sr1;
    for (int i = 0; i < 10000; i++) {
        spiSelect(dev->spip); spiSend(dev->spip, 1, &cmd); spiReceive(dev->spip, 1, &sr1); spiUnselect(dev->spip);
        if (!(sr1 & W25Q64_SR1_BUSY)) return 0;
        chThdSleepMilliseconds(1);
    }
    return -1;
}
static int w25q64jv_write_enable(struct w25q64jv_device *dev) {
    uint8_t cmd = W25Q64_CMD_WRITE_ENABLE;
    spiSelect(dev->spip); spiSend(dev->spip, 1, &cmd); spiUnselect(dev->spip);
    return 0;
}
int w25q64jv_init(struct w25q64jv_device *dev, SPIDriver *spip) {
    if (!dev || !spip) return -1;
    dev->spip = spip; dev->spicfg.cr1 = 0; dev->spicfg.cr2 = 0;
    return 0;
}
int w25q64jv_probe(struct w25q64jv_device *dev) {
    uint8_t cmd = W25Q64_CMD_JEDEC_ID, id[3];
    spiSelect(dev->spip); spiSend(dev->spip, 1, &cmd); spiReceive(dev->spip, 3, id); spiUnselect(dev->spip);
    return (id[0] == W25Q64_JEDEC_MFR_WINBOND) ? 0 : -1;
}
int w25q64jv_read(struct w25q64jv_device *dev, uint32_t addr, uint8_t *buf, uint32_t len) {
    if (!dev || !buf || len == 0) return -1;
    uint8_t cmd[4] = {W25Q64_CMD_READ_DATA, (uint8_t)(addr>>16), (uint8_t)(addr>>8), (uint8_t)addr};
    spiSelect(dev->spip); spiSend(dev->spip, 4, cmd); spiReceive(dev->spip, len, buf); spiUnselect(dev->spip);
    return 0;
}
int w25q64jv_write(struct w25q64jv_device *dev, uint32_t addr, const uint8_t *buf, uint32_t len) {
    if (!dev || !buf || len == 0) return -1;
    uint32_t offset = 0;
    while (offset < len) {
        uint32_t pr = W25Q64JV_PAGE_SIZE - ((addr+offset) % W25Q64JV_PAGE_SIZE);
        uint32_t chunk = (len-offset < pr) ? (len-offset) : pr;
        w25q64jv_write_enable(dev);
        uint8_t hdr[4] = {W25Q64_CMD_PAGE_PROGRAM, (uint8_t)((addr+offset)>>16), (uint8_t)((addr+offset)>>8), (uint8_t)(addr+offset)};
        spiSelect(dev->spip); spiSend(dev->spip, 4, hdr); spiSend(dev->spip, chunk, buf+offset); spiUnselect(dev->spip);
        if (w25q64jv_wait_ready(dev) != 0) return -1;
        offset += chunk;
    }
    return 0;
}
int w25q64jv_erase_sector(struct w25q64jv_device *dev, uint32_t addr) {
    w25q64jv_write_enable(dev);
    uint8_t cmd[4] = {W25Q64_CMD_SECTOR_ERASE, (uint8_t)(addr>>16), (uint8_t)(addr>>8), (uint8_t)addr};
    spiSelect(dev->spip); spiSend(dev->spip, 4, cmd); spiUnselect(dev->spip);
    return w25q64jv_wait_ready(dev);
}
