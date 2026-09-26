#include "w25q64jv.h"
#include "ch.h"
#include "hal.h"
#include <string.h>

#include "chibios.h"
#define CMD_READ_DATA      0x03
#define CMD_PAGE_PROGRAM   0x02
#define CMD_WRITE_ENABLE   0x06
#define CMD_JEDEC_ID       0x9F

void w25q64jv_init(struct w25q64jv_dev *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    uint8_t tx[1] = {CMD_JEDEC_ID};
    uint8_t rx[3];
    spiExchange(dev->bus_handle, 4, tx, rx);
}

int w25q64jv_read(struct w25q64jv_dev *dev, uint32_t addr, uint8_t *buf, size_t len) {
    uint8_t tx[4];
    tx[0] = CMD_READ_DATA;
    tx[1] = (addr >> 16) & 0xFF;
    tx[2] = (addr >> 8) & 0xFF;
    tx[3] = addr & 0xFF;
    spiExchange(dev->bus_handle, 4, tx, buf);
    return 0;
}

int w25q64jv_write(struct w25q64jv_dev *dev, uint32_t addr, const uint8_t *buf, size_t len) {
    uint8_t tx[4 + 256];
    tx[0] = CMD_WRITE_ENABLE;
    spiExchange(dev->bus_handle, 1, tx, NULL);
    tx[0] = CMD_PAGE_PROGRAM;
    tx[1] = (addr >> 16) & 0xFF;
    tx[2] = (addr >> 8) & 0xFF;
    tx[3] = addr & 0xFF;
    memcpy(&tx[4], buf, len);
    spiExchange(dev->bus_handle, 4 + len, tx, NULL);
    return 0;
}
