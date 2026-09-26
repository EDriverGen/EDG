#include "ds18b20.h"
#include <errno.h>
#include <string.h>
#include "xtimer.h"

#include <periph/gpio.h>
#define DS18B20_SKIP_ROM 0xCC
#define DS18B20_CONVERT_T 0x44
#define DS18B20_READ_SCRATCHPAD 0xBE

#define DS18B20_RESET_LOW_US 480
#define DS18B20_RESET_HIGH_US 480
#define DS18B20_PRESENCE_WAIT_US 60
#define DS18B20_PRESENCE_TIMEOUT_US 300
#define DS18B20_CONVERSION_MS 750
#define DS18B20_SLOT_US 120
#define DS18B20_RECOVERY_US 1
#define DS18B20_READ_VALID_US 15

static int ds18b20_reset(gpio_t pin)
{
    gpio_init(pin, GPIO_OUT);
    gpio_write(pin, 0);
    xtimer_usleep(DS18B20_RESET_LOW_US);
    gpio_init(pin, GPIO_IN);
    xtimer_usleep(DS18B20_PRESENCE_WAIT_US);
    uint32_t timeout = DS18B20_PRESENCE_TIMEOUT_US;
    while (gpio_read(pin) && timeout > 0) {
        xtimer_usleep(1);
        timeout--;
    }
    if (timeout == 0) {
        return -ENODEV;
    }
    timeout = DS18B20_PRESENCE_TIMEOUT_US;
    while (!gpio_read(pin) && timeout > 0) {
        xtimer_usleep(1);
        timeout--;
    }
    if (timeout == 0) {
        return -ENODEV;
    }
    return 0;
}

static void ds18b20_write_bit(gpio_t pin, int bit)
{
    gpio_init(pin, GPIO_OUT);
    gpio_write(pin, 0);
    if (bit) {
        xtimer_usleep(1);
        gpio_init(pin, GPIO_IN);
        xtimer_usleep(DS18B20_SLOT_US - 1);
    } else {
        xtimer_usleep(DS18B20_SLOT_US);
        gpio_init(pin, GPIO_IN);
    }
    xtimer_usleep(DS18B20_RECOVERY_US);
}

static int ds18b20_read_bit(gpio_t pin)
{
    int bit;
    gpio_init(pin, GPIO_OUT);
    gpio_write(pin, 0);
    xtimer_usleep(1);
    gpio_init(pin, GPIO_IN);
    xtimer_usleep(DS18B20_READ_VALID_US);
    bit = gpio_read(pin) ? 1 : 0;
    xtimer_usleep(DS18B20_SLOT_US - DS18B20_READ_VALID_US - 1);
    xtimer_usleep(DS18B20_RECOVERY_US);
    return bit;
}

static void ds18b20_write_byte(gpio_t pin, uint8_t byte)
{
    for (int i = 0; i < 8; i++) {
        ds18b20_write_bit(pin, byte & 1);
        byte >>= 1;
    }
}

static uint8_t ds18b20_read_byte(gpio_t pin)
{
    uint8_t byte = 0;
    for (int i = 0; i < 8; i++) {
        byte >>= 1;
        if (ds18b20_read_bit(pin)) {
            byte |= 0x80;
        }
    }
    return byte;
}

int ds18b20_init(ds18b20_t *dev, gpio_t pin)
{
    dev->pin = pin;
    int ret = ds18b20_reset(pin);
    if (ret != 0) {
        return ret;
    }
    return 0;
}

int ds18b20_read_temperature(ds18b20_t *dev, int32_t *raw)
{
    gpio_t pin = dev->pin;
    int ret;

    ret = ds18b20_reset(pin);
    if (ret != 0) return ret;
    ds18b20_write_byte(pin, DS18B20_SKIP_ROM);
    ds18b20_write_byte(pin, DS18B20_CONVERT_T);
    xtimer_msleep(DS18B20_CONVERSION_MS);

    ret = ds18b20_reset(pin);
    if (ret != 0) return ret;
    ds18b20_write_byte(pin, DS18B20_SKIP_ROM);
    ds18b20_write_byte(pin, DS18B20_READ_SCRATCHPAD);

    uint8_t lsb = ds18b20_read_byte(pin);
    uint8_t msb = ds18b20_read_byte(pin);

    int16_t raw16 = (int16_t)((msb << 8) | lsb);
    *raw = (int32_t)raw16 * 625 / 10;
    return 0;
}
