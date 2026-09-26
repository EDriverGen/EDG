#include "ds18b20.h"
#include "ch.h"
#include "hal.h"
#include "hal_pal.h"
#include <stdint.h>

#define DS18B20_SKIP_ROM 0xCC
#define DS18B20_CONVERT_T 0x44
#define DS18B20_READ_SCRATCHPAD 0xBE

static void ds18b20_write_bit(struct ds18b20_dev *dev, int bit)
{
    ioline_t line = PAL_LINE(GPIOA, 0);
    if (bit) {
        palClearPad(line.port, line.pad);
        chThdSleepMicroseconds(1);
        palSetPad(line.port, line.pad);
        chThdSleepMicroseconds(60);
    } else {
        palClearPad(line.port, line.pad);
        chThdSleepMicroseconds(60);
        palSetPad(line.port, line.pad);
        chThdSleepMicroseconds(1);
    }
}

static int ds18b20_read_bit(struct ds18b20_dev *dev)
{
    ioline_t line = PAL_LINE(GPIOA, 0);
    palClearPad(line.port, line.pad);
    chThdSleepMicroseconds(1);
    palSetPad(line.port, line.pad);
    chThdSleepMicroseconds(1);
    int bit = palReadLine(line);
    chThdSleepMicroseconds(60);
    return bit;
}

static void ds18b20_write_byte(struct ds18b20_dev *dev, uint8_t byte)
{
    for (int i = 0; i < 8; i++) {
        ds18b20_write_bit(dev, byte & 1);
        byte >>= 1;
    }
}

static uint8_t ds18b20_read_byte(struct ds18b20_dev *dev)
{
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        byte >>= 1;
        if (ds18b20_read_bit(dev))
            byte |= 0x80;
    }
    return byte;
}

static int ds18b20_reset(struct ds18b20_dev *dev)
{
    ioline_t line = PAL_LINE(GPIOA, 0);
    palClearPad(line.port, line.pad);
    chThdSleepMicroseconds(480);
    palSetPad(line.port, line.pad);
    chThdSleepMicroseconds(70);
    int presence = palReadLine(line);
    chThdSleepMicroseconds(410);
    return (presence == 0) ? 0 : -1;
}

int ds18b20_init(struct ds18b20_dev *dev, void *bus_handle)
{
    (void)bus_handle;
    dev->bus_handle = bus_handle;
    return ds18b20_reset(dev);
}

int ds18b20_read_temperature(struct ds18b20_dev *dev, int32_t *raw)
{
    int ret = ds18b20_reset(dev);
    if (ret != 0) return ret;
    ds18b20_write_byte(dev, DS18B20_SKIP_ROM);
    ds18b20_write_byte(dev, DS18B20_CONVERT_T);
    chThdSleepMilliseconds(750);
    ret = ds18b20_reset(dev);
    if (ret != 0) return ret;
    ds18b20_write_byte(dev, DS18B20_SKIP_ROM);
    ds18b20_write_byte(dev, DS18B20_READ_SCRATCHPAD);
    uint8_t lsb = ds18b20_read_byte(dev);
    uint8_t msb = ds18b20_read_byte(dev);
    int16_t raw_temp = (int16_t)((msb << 8) | lsb);
    *raw = (int32_t)raw_temp * 625 / 10;
    return 0;
}