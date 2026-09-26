#include "mcp3008.h"
#include <stdint.h>

#include "cmsis_rtx.h"
void mcp3008_init(struct mcp3008_dev *dev, void *bus_handle) {
    dev->hspi = (SPI_HandleTypeDef *)bus_handle;
}

int mcp3008_read_channel(struct mcp3008_dev *dev, uint8_t channel, uint16_t *out) {
    uint8_t tx[3];
    uint8_t rx[3];
    tx[0] = 0x01;
    tx[1] = 0x80 | (channel << 4);
    tx[2] = 0x00;
    
    if (HAL_SPI_TransmitReceive(dev->hspi, tx, rx, 3, 100) != HAL_OK) {
        return -1;
    }
    
    *out = ((uint16_t)(rx[1] & 0x03) << 8) | rx[2];
    return 0;
}
