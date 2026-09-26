#include "max31855.h"
#include "ch.h"
#include "hal.h"
#include <stdint.h>
#include <stddef.h>

#include "chibios.h"
void max31855_init(struct max31855_device *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    chThdSleepMilliseconds(200);
}

int max31855_read_temperatures(struct max31855_device *dev, int32_t *tc, int32_t *local) {
    uint8_t txbuf[4] = {0};
    uint8_t rxbuf[4] = {0};
    SPIConfig spicfg = {0};
    spiInit();
    spiStart((SPIDriver *)dev->bus_handle, &spicfg);
    spiSelect((SPIDriver *)dev->bus_handle);
    spiExchange((SPIDriver *)dev->bus_handle, 4, txbuf, rxbuf);
    spiUnselect((SPIDriver *)dev->bus_handle);
    spiStop((SPIDriver *)dev->bus_handle);

    uint32_t raw = ((uint32_t)rxbuf[0] << 24) | ((uint32_t)rxbuf[1] << 16) | ((uint32_t)rxbuf[2] << 8) | rxbuf[3];

    if (raw & (1 << 16)) {
        return 1;
    }

    uint16_t raw_tc = (raw >> 18) & 0x3FFF;
    int32_t tc_val;
    if (raw_tc & 0x2000) {
        tc_val = (int32_t)(raw_tc | 0xFFFFC000);
    } else {
        tc_val = (int32_t)raw_tc;
    }
    *tc = (tc_val * 250) / 1000;

    uint16_t raw_local = (raw >> 4) & 0xFFF;
    int32_t local_val;
    if (raw_local & 0x800) {
        local_val = (int32_t)(raw_local | 0xFFFFF000);
    } else {
        local_val = (int32_t)raw_local;
    }
    *local = (local_val * 625) / 10000;

    return 0;
}
