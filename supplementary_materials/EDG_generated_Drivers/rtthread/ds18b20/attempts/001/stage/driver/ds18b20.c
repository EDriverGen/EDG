#include "ds18b20.h"
#include "rtthread.h"
#include <stdint.h>
#include <errno.h>

#define DS18B20_SKIP_ROM 0xCC
#define DS18B20_CONVERT_T 0x44
#define DS18B20_READ_SCRATCHPAD 0xBE

#define DS18B20_RESET_LOW_US 480
#define DS18B20_RESET_HIGH_US 480
#define DS18B20_PRESENCE_WAIT_US 60
#define DS18B20_PRESENCE_TIMEOUT_US 240
#define DS18B20_CONVERSION_MS 750
#define DS18B20_READ_SLOT_US 60
#define DS18B20_SLOT_RECOVERY_US 1

static void ds18b20_delay_us(volatile uint32_t us)
{
    for (volatile uint32_t i = 0; i < us; i++) {
        __asm__ volatile ("nop");
    }
}

static int ds18b20_reset(struct ds18b20_device *dev)
{
    rt_pin_mode(dev->pin, PIN_MODE_OUTPUT);
    rt_pin_write(dev->pin, 0);
    ds18b20_delay_us(DS18B20_RESET_LOW_US);
    rt_pin_write(dev->pin, 1);
    rt_pin_mode(dev->pin, PIN_MODE_INPUT);
    ds18b20_delay_us(DS18B20_PRESENCE_WAIT_US);
    int presence = rt_pin_read(dev->pin);
    if (presence != 0) {
        return -ENODEV;
    }
    ds18b20_delay_us(DS18B20_PRESENCE_TIMEOUT_US);
    return 0;
}

static void ds18b20_write_bit(struct ds18b20_device *dev, int bit)
{
    rt_pin_mode(dev->pin, PIN_MODE_OUTPUT);
    rt_pin_write(dev->pin, 0);
    if (bit) {
        ds18b20_delay_us(1);
        rt_pin_write(dev->pin, 1);
        ds18b20_delay_us(DS18B20_READ_SLOT_US - 1);
    } else {
        ds18b20_delay_us(DS18B20_READ_SLOT_US);
        rt_pin_write(dev->pin, 1);
        ds18b20_delay_us(DS18B20_SLOT_RECOVERY_US);
    }
}

static int ds18b20_read_bit(struct ds18b20_device *dev)
{
    rt_pin_mode(dev->pin, PIN_MODE_OUTPUT);
    rt_pin_write(dev->pin, 0);
    ds18b20_delay_us(1);
    rt_pin_mode(dev->pin, PIN_MODE_INPUT);
    ds18b20_delay_us(DS18B20_READ_SLOT_US - 1);
    int bit = rt_pin_read(dev->pin);
    ds18b20_delay_us(DS18B20_SLOT_RECOVERY_US);
    return bit;
}

static void ds18b20_write_byte(struct ds18b20_device *dev, uint8_t byte)
{
    for (int i = 0; i < 8; i++) {
        ds18b20_write_bit(dev, byte & 0x01);
        byte >>= 1;
    }
}

static uint8_t ds18b20_read_byte(struct ds18b20_device *dev)
{
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        byte >>= 1;
        if (ds18b20_read_bit(dev)) {
            byte |= 0x80;
        }
    }
    return byte;
}

int ds18b20_init(struct ds18b20_device *dev, uint64_t pin)
{
    dev->pin = pin;
    int ret = ds18b20_reset(dev);
    if (ret != 0) {
        return ret;
    }
    return 0;
}

int ds18b20_read_temperature(struct ds18b20_device *dev, int32_t *raw)
{
    int ret;
    ret = ds18b20_reset(dev);
    if (ret != 0) return ret;
    ds18b20_write_byte(dev, DS18B20_SKIP_ROM);
    ds18b20_write_byte(dev, DS18B20_CONVERT_T);
    rt_thread_mdelay(DS18B20_CONVERSION_MS);
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