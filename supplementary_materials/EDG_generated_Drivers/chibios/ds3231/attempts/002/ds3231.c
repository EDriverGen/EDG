#include "ch.h"
#include "hal.h"
#include "ds3231.h"
#include <stdint.h>
#include <string.h>

#include "chibios.h"
#define DS3231_ADDR 0x68
#define DS3231_SECONDS_REG 0x00
#define DS3231_CONTROL_REG 0x0E
#define DS3231_STATUS_REG 0x0F
#define DS3231_TEMP_MSB_REG 0x11

static uint8_t bcd_to_dec(uint8_t bcd) {
    return (bcd >> 4) * 10 + (bcd & 0x0F);
}

static uint8_t dec_to_bcd(uint8_t dec) {
    return ((dec / 10) << 4) | (dec % 10);
}

int ds3231_init(struct ds3231_device *dev, void *bus_handle) {
    if (!dev || !bus_handle) return -1;
    dev->bus_handle = bus_handle;
    dev->i2c_addr = DS3231_ADDR;
    return 0;
}

int ds3231_get_time(struct ds3231_device *dev, struct ds3231_time *t) {
    if (!dev || !t) return -1;
    I2CDriver *i2cp = (I2CDriver *)dev->bus_handle;
    uint8_t txbuf[1] = {DS3231_SECONDS_REG};
    uint8_t rxbuf[7];
    msg_t status;

    i2cAcquireBus(i2cp);
    status = i2cMasterTransmitTimeout(i2cp, dev->i2c_addr, txbuf, 1, rxbuf, 7, TIME_MS2I(100));
    i2cReleaseBus(i2cp);

    if (status != MSG_OK) return -1;

    t->seconds = bcd_to_dec(rxbuf[0]);
    t->minutes = bcd_to_dec(rxbuf[1]);
    t->hours = bcd_to_dec(rxbuf[2]);
    t->day = rxbuf[3];
    t->date = bcd_to_dec(rxbuf[4]);
    t->month = bcd_to_dec(rxbuf[5] & 0x1F);
    t->year = bcd_to_dec(rxbuf[6]);

    return 0;
}

int ds3231_set_time(struct ds3231_device *dev, const struct ds3231_time *t) {
    if (!dev || !t) return -1;
    I2CDriver *i2cp = (I2CDriver *)dev->bus_handle;
    uint8_t txbuf[8];
    msg_t status;

    txbuf[0] = DS3231_SECONDS_REG;
    txbuf[1] = dec_to_bcd(t->seconds);
    txbuf[2] = dec_to_bcd(t->minutes);
    txbuf[3] = dec_to_bcd(t->hours);
    txbuf[4] = t->day;
    txbuf[5] = dec_to_bcd(t->date);
    txbuf[6] = dec_to_bcd(t->month);
    txbuf[7] = dec_to_bcd(t->year);

    i2cAcquireBus(i2cp);
    status = i2cMasterTransmitTimeout(i2cp, dev->i2c_addr, txbuf, 8, NULL, 0, TIME_MS2I(100));
    i2cReleaseBus(i2cp);

    return (status == MSG_OK) ? 0 : -1;
}
