#include "ds18b20.h"
#include "ch.h"
#include "hal.h"
#include "hal_pal.h"
#include <stdint.h>

#define DS18B20_SKIP_ROM 0xCC
#define DS18B20_CONVERT_T 0x44
#define DS18B20_READ_SCRATCHPAD 0xBE

static inline void ds18b20_write_bit(ioline_t line, int bit)
{
    if (bit) {
        palClearPad(PAL_PORT(line), PAL_PAD(line));
        chThdSleepMicroseconds(1);
        palSetPadMode(PAL_PORT(line), PAL_PAD(line), PAL_MODE_INPUT);
        chThdSleepMicroseconds(60);
    } else {
        palClearPad(PAL_PORT(line), PAL_PAD(line));
        chThdSleepMicroseconds(60);
        palSetPadMode(PAL_PORT(line), PAL_PAD(line), PAL_MODE_INPUT);
        chThdSleepMicroseconds(1);
    }
}

static inline int ds18b20_read_bit(ioline_t line)
{
    int bit;
    palClearPad(PAL_PORT(line), PAL_PAD(line));
    chThdSleepMicroseconds(1);
    palSetPadMode(PAL_PORT(line), PAL_PAD(line), PAL_MODE_INPUT);
    chThdSleepMicroseconds(1);
    bit = palReadLine(line);
    chThdSleepMicroseconds(50);
    return bit;
}

static void ds18b20_write_byte(ioline_t line, uint8_t byte)
{
    for (int i = 0; i < 8; i++) {
        ds18b20_write_bit(line, byte & 0x01);
        byte >>= 1;
    }
}

static uint8_t ds18b20_read_byte(ioline_t line)
{
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        byte >>= 1;
        if (ds18b20_read_bit(line))
            byte |= 0x80;
    }
    return byte;
}

static int ds18b20_reset(ioline_t line)
{
    palSetPadMode(PAL_PORT(line), PAL_PAD(line), PAL_MODE_OUTPUT_PUSHPULL);
    palClearPad(PAL_PORT(line), PAL_PAD(line));
    chThdSleepMicroseconds(480);
    palSetPadMode(PAL_PORT(line), PAL_PAD(line), PAL_MODE_INPUT);
    chThdSleepMicroseconds(70);
    int presence = palReadLine(line);
    chThdSleepMicroseconds(410);
    return (presence == 0) ? 0 : -1;
}

int ds18b20_init(struct ds18b20_dev *dev, void *bus_handle)
{
    dev->bus_handle = bus_handle;
    ioline_t line = PAL_LINE(GPIOA, 0);
    return ds18b20_reset(line);
}

int ds18b20_read_temperature(struct ds18b20_dev *dev, int32_t *raw)
{
    (void)dev;
    ioline_t line = PAL_LINE(GPIOA, 0);
    int ret;

    ret = ds18b20_reset(line);
    if (ret != 0)
        return ret;

    ds18b20_write_byte(line, DS18B20_SKIP_ROM);
    ds18b20_write_byte(line, DS18B20_CONVERT_T);

    chThdSleepMilliseconds(750);

    ret = ds18b20_reset(line);
    if (ret != 0)
        return ret;

    ds18b20_write_byte(line, DS18B20_SKIP_ROM);
    ds18b20_write_byte(line, DS18B20_READ_SCRATCHPAD);

    uint8_t lsb = ds18b20_read_byte(line);
    uint8_t msb = ds18b20_read_byte(line);

    int16_t raw_temp = (int16_t)((msb << 8) | lsb);
    if (raw_temp & 0x800)
        raw_temp |= 0xF000;

    *raw = (int32_t)raw_temp * 625 / 10;
    return 0;
}