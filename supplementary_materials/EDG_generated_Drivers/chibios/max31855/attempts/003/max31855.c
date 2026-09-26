#include "max31855.h"
#include "ch.h"
#include "hal.h"
#include <stddef.h>
#include <stdint.h>

#include "chibios.h"
#define MAX31855_CS_PORT GPIOA
#define MAX31855_CS_PAD 4

void max31855_init(struct max31855_device *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    palSetPadMode(MAX31855_CS_PORT, MAX31855_CS_PAD, PAL_MODE_OUTPUT_PUSHPULL);
    palSetPad(MAX31855_CS_PORT, MAX31855_CS_PAD);
    chThdSleepMilliseconds(200);
}

static int32_t extract_thermocouple(uint32_t raw) {
    int32_t raw14 = (int32_t)((raw >> 18) & 0x3FFF);
    if (raw14 & 0x2000) {
        raw14 |= ~0x3FFF;
    }
    return (raw14 * 250) / 1000;
}

static int32_t extract_internal(uint32_t raw) {
    int32_t raw12 = (int32_t)((raw >> 4) & 0xFFF);
    if (raw12 & 0x800) {
        raw12 |= ~0xFFF;
    }
    return (raw12 * 625) / 10000;
}

int max31855_read_temperatures(struct max31855_device *dev, int32_t *tc, int32_t *local) {
    uint8_t txbuf[4] = {0};
    uint8_t rxbuf[4] = {0};
    SPIConfig spicfg = {0};
    spicfg.ssport = MAX31855_CS_PORT;
    spicfg.sspad = MAX31855_CS_PAD;
    spiInit();
    spiStart((SPIDriver *)dev->bus_handle, &spicfg);
    spiSelect((SPIDriver *)dev->bus_handle);
    spiExchange((SPIDriver *)dev->bus_handle, 4, txbuf, rxbuf);
    spiUnselect((SPIDriver *)dev->bus_handle);
    spiStop((SPIDriver *)dev->bus_handle);

    uint32_t raw = ((uint32_t)rxbuf[0] << 24) | ((uint32_t)rxbuf[1] << 16) | ((uint32_t)rxbuf[2] << 8) | rxbuf[3];

    if (raw & 0x10000) {
        *tc = 0;
        *local = 0;
        return 1;
    }

    *tc = extract_thermocouple(raw);
    *local = extract_internal(raw);
    return 0;
}
