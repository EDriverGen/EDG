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
    
    /* JEDEC ID read: send command, then read 3 bytes */
    uint8_t tx[4] = {CMD_JEDEC_ID, 0, 0, 0};
    uint8_t rx[4] = {0};
    spiExchange(dev->bus_handle, 4, tx, rx);
}

int w25q64jv_read(struct w25q64jv_dev *dev, uint32_t addr, uint8_t *buf, size_t len) {
    if (len == 0) return 0;
    
    /* Build command: CMD_READ_DATA + 3-byte address (big-endian) */
    uint8_t cmd[4];
    cmd[0] = CMD_READ_DATA;
    cmd[1] = (addr >> 16) & 0xFF;
    cmd[2] = (addr >> 8) & 0xFF;
    cmd[3] = addr & 0xFF;
    
    /* Send command + dummy bytes for the address phase, then read data */
    /* We need to send 4 bytes (command+address) and then read len bytes */
    /* Use a single exchange: send cmd+address followed by dummy bytes for the read */
    size_t total = 4 + len;
    uint8_t tx[total];
    uint8_t rx[total];
    memcpy(tx, cmd, 4);
    memset(tx + 4, 0, len);
    
    spiExchange(dev->bus_handle, total, tx, rx);
    
    /* Copy received data (starting after the command echo) */
    memcpy(buf, rx + 4, len);
    
    return 0;
}

int w25q64jv_write(struct w25q64jv_dev *dev, uint32_t addr, const uint8_t *buf, size_t len) {
    if (len == 0) return 0;
    if (len > 256) len = 256; /* page program max 256 bytes */
    
    /* Write enable */
    uint8_t we_cmd = CMD_WRITE_ENABLE;
    spiExchange(dev->bus_handle, 1, &we_cmd, NULL);
    
    /* Page program: command + 3-byte address + data */
    size_t total = 4 + len;
    uint8_t tx[total];
    tx[0] = CMD_PAGE_PROGRAM;
    tx[1] = (addr >> 16) & 0xFF;
    tx[2] = (addr >> 8) & 0xFF;
    tx[3] = addr & 0xFF;
    memcpy(tx + 4, buf, len);
    
    spiExchange(dev->bus_handle, total, tx, NULL);
    
    return 0;
}
