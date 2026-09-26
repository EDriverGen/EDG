#include "max31855.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

#include <os/os_time.h>
#define MAX31855_CS_PIN 0

int max31855_init(struct max31855_dev *dev, void *bus_handle) {
    assert(dev);
    assert(bus_handle);
    dev->bus_handle = bus_handle;
    dev->cs_pin = MAX31855_CS_PIN;
    os_time_delay(OS_TICKS_PER_SEC / 5);
    return 0;
}

int max31855_read_temperatures(struct max31855_dev *dev, int32_t *thermocouple_val, int32_t *internal_val) {
    assert(dev);
    assert(thermocouple_val);
    assert(internal_val);
    uint8_t tx_buf[4] = {0};
    uint8_t rx_buf[4];
    int rc = hal_spi_txrx(0, tx_buf, rx_buf, 4);
    if (rc != 0) {
        return -1;
    }
    uint32_t raw = ((uint32_t)rx_buf[0] << 24) | ((uint32_t)rx_buf[1] << 16) | ((uint32_t)rx_buf[2] << 8) | rx_buf[3];
    if (raw & 0x00010000) {
        return 1;
    }
    uint16_t raw_14 = (raw >> 18) & 0x3FFF;
    int32_t sign_14 = (raw_14 >> 13) & 1;
    int32_t thermocouple_raw = sign_14 ? (-8192 + (int32_t)(raw_14 & 0x1FFF)) : (int32_t)(raw_14 & 0x1FFF);
    *thermocouple_val = (thermocouple_raw * 250) / 1000;
    uint16_t raw_12 = (raw >> 4) & 0xFFF;
    int32_t sign_12 = (raw_12 >> 11) & 1;
    int32_t internal_raw = sign_12 ? (-2048 + (int32_t)(raw_12 & 0x7FF)) : (int32_t)(raw_12 & 0x7FF);
    *internal_val = (internal_raw * 625) / 10000;
    return 0;
}
