#include "pcf8574.h"
#include "hal.h"
#include <string.h>

#include "hal_i2c.h"
#define PCF8574_I2C_ADDR 0x20
#define I2C_TIMEOUT_MS 100

void pcf8574_init(struct pcf8574_device *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = PCF8574_I2C_ADDR;
}

void pcf8574_read_port(struct pcf8574_device *dev, uint8_t *p0, uint8_t *p1, uint8_t *p2, uint8_t *p3, uint8_t *p4, uint8_t *p5, uint8_t *p6, uint8_t *p7)
{
    I2CDriver *i2cp = (I2CDriver *)dev->bus_handle;
    uint8_t rx_byte = 0;
    msg_t status;

    i2cAcquireBus(i2cp);
    status = i2cMasterTransmitTimeout(i2cp, dev->i2c_addr, NULL, 0, &rx_byte, 1, MS2ST(I2C_TIMEOUT_MS));
    i2cReleaseBus(i2cp);

    if (status != MSG_OK) {
        *p0 = 0; *p1 = 0; *p2 = 0; *p3 = 0;
        *p4 = 0; *p5 = 0; *p6 = 0; *p7 = 0;
        return;
    }

    *p0 = (rx_byte >> 0) & 1;
    *p1 = (rx_byte >> 1) & 1;
    *p2 = (rx_byte >> 2) & 1;
    *p3 = (rx_byte >> 3) & 1;
    *p4 = (rx_byte >> 4) & 1;
    *p5 = (rx_byte >> 5) & 1;
    *p6 = (rx_byte >> 6) & 1;
    *p7 = (rx_byte >> 7) & 1;
}
