/*
 * DS18B20 sensor driver for RT-Thread (GPIO bit-bang 1-Wire)
 */
#include "ds18b20_ref.h"

static const uint8_t crc8_table[256] = {
    0,94,188,226,97,63,221,131,194,156,126,32,163,253,31,65,
    157,195,33,127,252,162,64,30,95,1,227,189,62,96,130,220,
    35,125,159,193,66,28,254,160,225,191,93,3,128,222,60,98,
    190,224,2,92,223,129,99,61,124,34,192,158,29,67,161,255,
    70,24,250,164,39,121,155,197,132,218,56,102,229,187,89,7,
    219,133,103,57,186,228,6,88,25,71,165,251,120,38,196,154,
    101,59,217,135,4,90,184,230,167,249,27,69,198,152,122,36,
    248,166,68,26,153,199,37,123,58,100,134,216,91,5,231,185,
    140,210,48,110,237,179,81,15,78,16,242,172,47,113,147,205,
    17,79,173,243,112,46,204,146,211,141,111,49,178,236,14,80,
    175,241,19,77,206,144,114,44,109,51,209,143,12,82,176,238,
    50,108,142,208,83,13,239,177,240,174,76,18,145,207,45,115,
    202,148,118,40,171,245,23,73,8,86,180,234,105,55,213,139,
    87,9,235,181,54,104,138,212,149,203,41,119,244,170,72,22,
    233,183,85,11,136,214,52,106,43,117,151,201,74,20,246,168,
    116,42,200,150,21,75,169,247,182,232,10,84,215,137,107,53
};

static uint8_t ds18b20_crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++)
        crc = crc8_table[crc ^ data[i]];
    return crc;
}

static int ds18b20_reset(struct ds18b20_device *dev)
{
    int present;
    rt_pin_mode(dev->data_pin, PIN_MODE_OUTPUT);
    rt_pin_write(dev->data_pin, PIN_LOW);
    rt_hw_us_delay(480);
    rt_pin_mode(dev->data_pin, PIN_MODE_INPUT);
    rt_hw_us_delay(60);
    present = (rt_pin_read(dev->data_pin) == PIN_LOW) ? 0 : -1;
    rt_hw_us_delay(420);
    return present;
}

static void ds18b20_write_bit(struct ds18b20_device *dev, int bit)
{
    rt_pin_mode(dev->data_pin, PIN_MODE_OUTPUT);
    rt_pin_write(dev->data_pin, PIN_LOW);
    if (bit) {
        rt_hw_us_delay(5);
        rt_pin_write(dev->data_pin, PIN_HIGH);
        rt_hw_us_delay(55);
    } else {
        rt_hw_us_delay(60);
        rt_pin_write(dev->data_pin, PIN_HIGH);
        rt_hw_us_delay(5);
    }
}

static int ds18b20_read_bit(struct ds18b20_device *dev)
{
    int val;
    rt_pin_mode(dev->data_pin, PIN_MODE_OUTPUT);
    rt_pin_write(dev->data_pin, PIN_LOW);
    rt_hw_us_delay(2);
    rt_pin_mode(dev->data_pin, PIN_MODE_INPUT);
    rt_hw_us_delay(12);
    val = (rt_pin_read(dev->data_pin) == PIN_HIGH) ? 1 : 0;
    rt_hw_us_delay(50);
    return val;
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
    uint8_t val = 0;
    for (int i = 0; i < 8; i++)
        val |= (ds18b20_read_bit(dev) << i);
    return val;
}

rt_err_t ds18b20_init(struct ds18b20_device *dev, rt_base_t data_pin)
{
    if (dev == RT_NULL) return -RT_EINVAL;
    dev->data_pin = data_pin;
    if (ds18b20_reset(dev) != 0) return -RT_EIO;
    return RT_EOK;
}

rt_err_t ds18b20_read_temp(struct ds18b20_device *dev, int32_t *temp_x100)
{
    uint8_t buf[9];
    rt_base_t level;
    if (dev == RT_NULL || temp_x100 == RT_NULL) return -RT_EINVAL;

    level = rt_hw_interrupt_disable();
    if (ds18b20_reset(dev) != 0) { rt_hw_interrupt_enable(level); return -RT_EIO; }
    ds18b20_write_byte(dev, DS18B20_CMD_SKIP_ROM);
    ds18b20_write_byte(dev, DS18B20_CMD_CONVERT_T);
    rt_hw_interrupt_enable(level);

    rt_thread_mdelay(DS18B20_CONVERT_WAIT_MS);

    level = rt_hw_interrupt_disable();
    if (ds18b20_reset(dev) != 0) { rt_hw_interrupt_enable(level); return -RT_EIO; }
    ds18b20_write_byte(dev, DS18B20_CMD_SKIP_ROM);
    ds18b20_write_byte(dev, DS18B20_CMD_READ_SCRATCH);
    for (int i = 0; i < 9; i++) buf[i] = ds18b20_read_byte(dev);
    rt_hw_interrupt_enable(level);

    /* verify CRC */
    if (ds18b20_crc8(buf, 8) != buf[8]) return -RT_EIO;
    int16_t raw = (int16_t)((uint16_t)buf[1] << 8 | buf[0]);
    *temp_x100 = (int32_t)raw * 100 / 16;  /* 0.0625 degC per LSB */
    return RT_EOK;
}

/* Pure-logic scratchpad decoder shared with host-side logic tests. */
int ds18b20_decode_scratchpad(const unsigned char scratchpad[9],
                              int *temp_x16,
                              unsigned char *resolution_bits,
                              int *crc_ok)
{
    if (scratchpad == 0) return -1;
    int crc_pass = (ds18b20_crc8((const uint8_t *)scratchpad, 8) == scratchpad[8]);
    if (crc_ok) *crc_ok = crc_pass;
    if (temp_x16) {
        int16_t raw = (int16_t)((unsigned short)scratchpad[1] << 8
                                 | scratchpad[0]);
        *temp_x16 = (int)raw;
    }
    if (resolution_bits) {
        /* Config register at byte 4: bits 6:5 select resolution.
         *   00 → 9, 01 → 10, 10 → 11, 11 → 12 */
        unsigned char cfg = (unsigned char)((scratchpad[4] >> 5) & 0x03);
        *resolution_bits = (unsigned char)(9 + cfg);
    }
    return crc_pass ? 0 : -1;
}
