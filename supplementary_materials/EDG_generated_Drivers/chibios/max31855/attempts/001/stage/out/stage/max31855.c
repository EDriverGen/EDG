#include "max31855.h"
#include "ch.h"
#include "hal.h"
#include <stddef.h>

#include "chibios.h"
void max31855_init(struct max31855_device *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    chThdSleepMilliseconds(200);
}

int max31855_read_temperatures(struct max31855_device *dev, int32_t *tc, int32_t *local) {
    uint8_t txbuf[4] = {0};
    uint8_t rxbuf[4] = {0};
    SPIDriver *spi = (SPIDriver *)dev->bus_handle;
    
    spiAcquireBus(spi);
    spiSelect(spi);
    spiExchange(spi, 4, txbuf, rxbuf);
    spiUnselect(spi);
    spiReleaseBus(spi);
    
    uint32_t raw = ((uint32_t)rxbuf[0] << 24) | ((uint32_t)rxbuf[1] << 16) | ((uint32_t)rxbuf[2] << 8) | rxbuf[3];
    
    if (raw & 0x00010000) {
        return 1;
    }
    
    int16_t raw_tc = (int16_t)((raw >> 18) & 0x3FFF);
    if (raw_tc & 0x2000) {
        raw_tc |= 0xC000;
    }
    int32_t tc_val = ((int32_t)raw_tc) * 250 / 1000;
    *tc = tc_val;
    
    int16_t raw_local = (int16_t)((raw >> 4) & 0xFFF);
    if (raw_local & 0x0800) {
        raw_local |= 0xF000;
    }
    int32_t local_val = ((int32_t)raw_local) * 625 / 10000;
    *local = local_val;
    
    return 0;
}
