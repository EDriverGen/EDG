#include "ds18b20_ref.h"

static uint8_t ds18b20_crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        uint8_t in = data[i];
        for (uint8_t b = 0; b < 8; b++) {
            uint8_t mix = (crc ^ in) & 0x01U;
            crc >>= 1;
            if (mix) crc ^= 0x8CU;
            in >>= 1;
        }
    }
    return crc;
}

static int ds18b20_reset(struct ds18b20_device *dev)
{
    int present;
    hal_gpio_init_out(dev->data_pin, 0);
    os_cputime_delay_usecs(480);
    hal_gpio_init_in(dev->data_pin, 0);
    os_cputime_delay_usecs(60);
    present = hal_gpio_read(dev->data_pin) == 0 ? 0 : -1;
    os_cputime_delay_usecs(420);
    return present;
}

static void ds18b20_write_bit(struct ds18b20_device *dev, int bit)
{
    hal_gpio_init_out(dev->data_pin, 0);
    if (bit) {
        os_cputime_delay_usecs(5);
        hal_gpio_write(dev->data_pin, 1);
        hal_gpio_init_in(dev->data_pin, 0);
        os_cputime_delay_usecs(55);
    } else {
        os_cputime_delay_usecs(60);
        hal_gpio_write(dev->data_pin, 1);
        hal_gpio_init_in(dev->data_pin, 0);
        os_cputime_delay_usecs(5);
    }
}

static int ds18b20_read_bit(struct ds18b20_device *dev)
{
    int val;
    hal_gpio_init_out(dev->data_pin, 0);
    os_cputime_delay_usecs(2);
    hal_gpio_init_in(dev->data_pin, 0);
    os_cputime_delay_usecs(12);
    val = hal_gpio_read(dev->data_pin) ? 1 : 0;
    os_cputime_delay_usecs(50);
    return val;
}

static void ds18b20_write_byte(struct ds18b20_device *dev, uint8_t byte)
{
    for (int i = 0; i < 8; i++) {
        ds18b20_write_bit(dev, byte & 0x01U);
        byte >>= 1;
    }
}

static uint8_t ds18b20_read_byte(struct ds18b20_device *dev)
{
    uint8_t val = 0;
    for (int i = 0; i < 8; i++) {
        val |= (uint8_t)(ds18b20_read_bit(dev) << i);
    }
    return val;
}

int ds18b20_init(struct ds18b20_device *dev, int data_pin)
{
    if (dev == 0) return -1;
    dev->data_pin = data_pin;
    return ds18b20_reset(dev);
}

int ds18b20_read_temp(struct ds18b20_device *dev, int32_t *temp_x100)
{
    uint8_t buf[9];
    if (dev == 0 || temp_x100 == 0) return -1;
    if (ds18b20_reset(dev) != 0) return -1;
    ds18b20_write_byte(dev, DS18B20_CMD_SKIP_ROM);
    ds18b20_write_byte(dev, DS18B20_CMD_CONVERT_T);
    os_time_delay(DS18B20_CONVERT_WAIT_MS);
    if (ds18b20_reset(dev) != 0) return -1;
    ds18b20_write_byte(dev, DS18B20_CMD_SKIP_ROM);
    ds18b20_write_byte(dev, DS18B20_CMD_READ_SCRATCH);
    for (int i = 0; i < 9; i++) buf[i] = ds18b20_read_byte(dev);
    if (ds18b20_crc8(buf, 8) != buf[8]) return -1;
    *temp_x100 = (int32_t)((int16_t)(((uint16_t)buf[1] << 8) | buf[0])) * 100 / 16;
    return 0;
}

int ds18b20_decode_scratchpad(const unsigned char scratchpad[9],
                              int *temp_x16,
                              unsigned char *resolution_bits,
                              int *crc_ok)
{
    int pass;
    if (scratchpad == 0) return -1;
    pass = ds18b20_crc8((const uint8_t *)scratchpad, 8) == scratchpad[8];
    if (crc_ok) *crc_ok = pass;
    if (temp_x16) *temp_x16 = (int16_t)(((uint16_t)scratchpad[1] << 8) | scratchpad[0]);
    if (resolution_bits) *resolution_bits = (unsigned char)(9 + ((scratchpad[4] >> 5) & 0x03U));
    return pass ? 0 : -1;
}
