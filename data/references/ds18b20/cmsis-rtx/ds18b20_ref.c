#include "ds18b20_ref.h"

static void ds18b20_set_output(struct ds18b20_device *dev)
{
    GPIO_InitTypeDef gpio;
    gpio.Pin = dev->pin;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(dev->port, &gpio);
}

static void ds18b20_set_input(struct ds18b20_device *dev)
{
    GPIO_InitTypeDef gpio;
    gpio.Pin = dev->pin;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(dev->port, &gpio);
}

static void ds18b20_delay_us(uint32_t us)
{
    (void)us;
}

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
    ds18b20_set_output(dev);
    HAL_GPIO_WritePin(dev->port, dev->pin, GPIO_PIN_RESET);
    ds18b20_delay_us(480);
    ds18b20_set_input(dev);
    ds18b20_delay_us(60);
    present = HAL_GPIO_ReadPin(dev->port, dev->pin) == GPIO_PIN_RESET ? 0 : -1;
    ds18b20_delay_us(420);
    return present;
}

static void ds18b20_write_bit(struct ds18b20_device *dev, int bit)
{
    ds18b20_set_output(dev);
    HAL_GPIO_WritePin(dev->port, dev->pin, GPIO_PIN_RESET);
    if (bit) {
        ds18b20_delay_us(5);
        HAL_GPIO_WritePin(dev->port, dev->pin, GPIO_PIN_SET);
        ds18b20_set_input(dev);
        ds18b20_delay_us(55);
    } else {
        ds18b20_delay_us(60);
        HAL_GPIO_WritePin(dev->port, dev->pin, GPIO_PIN_SET);
        ds18b20_set_input(dev);
        ds18b20_delay_us(5);
    }
}

static int ds18b20_read_bit(struct ds18b20_device *dev)
{
    int val;
    ds18b20_set_output(dev);
    HAL_GPIO_WritePin(dev->port, dev->pin, GPIO_PIN_RESET);
    ds18b20_delay_us(2);
    ds18b20_set_input(dev);
    ds18b20_delay_us(12);
    val = HAL_GPIO_ReadPin(dev->port, dev->pin) == GPIO_PIN_SET ? 1 : 0;
    ds18b20_delay_us(50);
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

int ds18b20_init(struct ds18b20_device *dev, GPIO_TypeDef *port, uint16_t pin)
{
    if (dev == 0 || port == 0 || pin == 0) {
        return -1;
    }
    dev->port = port;
    dev->pin = pin;
    return ds18b20_reset(dev);
}

int ds18b20_read_temp(struct ds18b20_device *dev, int32_t *temp_x100)
{
    uint8_t buf[9];
    if (dev == 0 || temp_x100 == 0) {
        return -1;
    }
    if (ds18b20_reset(dev) != 0) return -1;
    ds18b20_write_byte(dev, DS18B20_CMD_SKIP_ROM);
    ds18b20_write_byte(dev, DS18B20_CMD_CONVERT_T);
    HAL_Delay(DS18B20_CONVERT_WAIT_MS);
    if (ds18b20_reset(dev) != 0) return -1;
    ds18b20_write_byte(dev, DS18B20_CMD_SKIP_ROM);
    ds18b20_write_byte(dev, DS18B20_CMD_READ_SCRATCH);
    for (int i = 0; i < 9; i++) {
        buf[i] = ds18b20_read_byte(dev);
    }
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
    if (temp_x16) {
        int16_t raw = (int16_t)(((uint16_t)scratchpad[1] << 8) | scratchpad[0]);
        *temp_x16 = raw;
    }
    if (resolution_bits) {
        *resolution_bits = (unsigned char)(9 + ((scratchpad[4] >> 5) & 0x03U));
    }
    return pass ? 0 : -1;
}
