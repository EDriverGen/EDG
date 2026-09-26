#include "max31855.h"
#include <stdint.h>
#include <stddef.h>

#include "riot.h"
int max31855_init(max31855_t *dev, spi_t bus, spi_cs_t cs) {
    dev->bus = bus;
    dev->cs = cs;
    spi_init_cs(bus, cs);
    ztimer_sleep(ZTIMER_MSEC, 200);
    return 0;
}

int max31855_read_temperatures(max31855_t *dev, int32_t *thermocouple_val, int32_t *internal_val) {
    uint8_t buf[4];
    spi_transfer_bytes(dev->bus, dev->cs, false, NULL, buf, 4);
    uint32_t raw = ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | buf[3];
    
    if (raw & (1 << 16)) {
        return 1;
    }
    
    uint16_t raw_thermo = (raw >> 18) & 0x3FFF;
    int32_t thermo_sign = (raw_thermo >> 13) & 1;
    int32_t thermo_mag = raw_thermo & 0x1FFF;
    int32_t thermo_signed = thermo_sign ? (thermo_mag - 0x2000) : thermo_mag;
    *thermocouple_val = (thermo_signed * 250) / 1000;
    
    uint16_t raw_internal = (raw >> 4) & 0xFFF;
    int32_t internal_sign = (raw_internal >> 11) & 1;
    int32_t internal_mag = raw_internal & 0x7FF;
    int32_t internal_signed = internal_sign ? (internal_mag - 0x800) : internal_mag;
    *internal_val = (internal_signed * 625) / 10000;
    
    return 0;
}
