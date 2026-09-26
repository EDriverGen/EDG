#include "ds18b20.h"
#include "ch.h"
#include "hal.h"
#include "hal_pal.h"
#include <stdint.h>
#include <stddef.h>

#define DS18B20_SKIP_ROM 0xCC
#define DS18B20_CONVERT_T 0x44
#define DS18B20_READ_SCRATCHPAD 0xBE

#define DS18B20_RESET_LOW_US 480
#define DS18B20_PRESENCE_WAIT_US 480
#define DS18B20_CONVERSION_DELAY_MS 750

static int ds18b20_reset_pulse(struct ds18b20_dev *dev)
{
    (void)dev;
    ioline_t line = PAL_LINE(GPIOA, 0);
    palClearPad(line.port, line.pad);
    chThdSleepMicroseconds(DS18B20_RESET_LOW_US);
    palSetPadMode(line.port, line.pad, PAL_MODE_INPUT_PULLUP);
    chThdSleepMicroseconds(DS18B20_PRESENCE_WAIT_US);
    if (palReadLine(line) == 0) {
        return 0;
    }
    return -1;
}

static void ds18b20_write_byte(struct ds18b20_dev *dev, uint8_t byte)
{
    (void)dev;
    ioline_t line = PAL_LINE(GPIOA, 0);
    for (int i = 0; i < 8; i++) {
        if (byte & (1 << i)) {
            palClearPad(line.port, line.pad);
            chThdSleepMicroseconds(1);
            palSetPadMode(line.port, line.pad, PAL_MODE_INPUT_PULLUP);
            chThdSleepMicroseconds(60);
        } else {
            palClearPad(line.port, line.pad);
            chThdSleepMicroseconds(60);
            palSetPadMode(line.port, line.pad, PAL_MODE_INPUT_PULLUP);
            chThdSleepMicroseconds(1);
        }
        chThdSleepMicroseconds(1);
    }
}

static uint8_t ds18b20_read_byte(struct ds18b20_dev *dev)
{
    (void)dev;
    ioline_t line = PAL_LINE(GPIOA, 0);
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        palClearPad(line.port, line.pad);
        chThdSleepMicroseconds(1);
        palSetPadMode(line.port, line.pad, PAL_MODE_INPUT_PULLUP);
        chThdSleepMicroseconds(1);
        if (palReadLine(line)) {
            byte |= (1 << i);
        }
        chThdSleepMicroseconds(60);
    }
    return byte;
}

int ds18b20_init(struct ds18b20_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = 0;
    ioline_t line = PAL_LINE(GPIOA, 0);
    palSetPadMode(line.port, line.pad, PAL_MODE_OUTPUT_PUSHPULL);
    int ret = ds18b20_reset_pulse(dev);
    if (ret != 0) {
        return -1;
    }
    return 0;
}

int ds18b20_read_temperature(struct ds18b20_dev *dev, int32_t *raw)
{
    (void)dev;
    ioline_t line = PAL_LINE(GPIOA, 0);
    palSetPadMode(line.port, line.pad, PAL_MODE_OUTPUT_PUSHPULL);
    int ret = ds18b20_reset_pulse(dev);
    if (ret != 0) {
        return -1;
    }
    ds18b20_write_byte(dev, DS18B20_SKIP_ROM);
    ds18b20_write_byte(dev, DS18B20_CONVERT_T);
    chThdSleepMilliseconds(DS18B20_CONVERSION_DELAY_MS);
    ret = ds18b20_reset_pulse(dev);
    if (ret != 0) {
        return -1;
    }
    ds18b20_write_byte(dev, DS18B20_SKIP_ROM);
    ds18b20_write_byte(dev, DS18B20_READ_SCRATCHPAD);
    uint8_t lsb = ds18b20_read_byte(dev);
    uint8_t msb = ds18b20_read_byte(dev);
    int16_t raw_temp = (int16_t)((msb << 8) | lsb);
    if (raw_temp & 0x800) {
        raw_temp |= 0xF000;
    }
    *raw = (int32_t)raw_temp * 625 / 10;
    return 0;
}