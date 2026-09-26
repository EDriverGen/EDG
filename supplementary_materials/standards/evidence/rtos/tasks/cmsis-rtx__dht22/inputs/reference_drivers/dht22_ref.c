#include "dht22_ref.h"

static void dht22_set_output(struct dht22_device *dev)
{
    GPIO_InitTypeDef gpio;
    gpio.Pin = dev->pin;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(dev->port, &gpio);
}

static void dht22_set_input(struct dht22_device *dev)
{
    GPIO_InitTypeDef gpio;
    gpio.Pin = dev->pin;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(dev->port, &gpio);
}

static void dht22_delay_us(uint32_t us)
{
    (void)us;
}

int dht22_init(struct dht22_device *dev, GPIO_TypeDef *port, uint16_t pin)
{
    if (dev == 0 || port == 0 || pin == 0) {
        return -1;
    }
    dev->port = port;
    dev->pin = pin;
    dht22_set_output(dev);
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
    return 0;
}

static int dht22_wait_level(struct dht22_device *dev, GPIO_PinState level, int max_us)
{
    int us = 0;
    while (HAL_GPIO_ReadPin(dev->port, dev->pin) == level) {
        dht22_delay_us(1);
        if (++us > max_us) {
            return -1;
        }
    }
    return us;
}

int dht22_read(struct dht22_device *dev, int16_t *temp_x10, uint16_t *humidity_x10)
{
    uint8_t data[5] = {0};
    if (dev == 0 || temp_x10 == 0 || humidity_x10 == 0) {
        return -1;
    }
    dht22_set_output(dev);
    HAL_GPIO_WritePin(dev->port, dev->pin, GPIO_PIN_RESET);
    HAL_Delay(2);
    HAL_GPIO_WritePin(dev->port, dev->pin, GPIO_PIN_SET);
    dht22_delay_us(30);
    dht22_set_input(dev);

    if (dht22_wait_level(dev, GPIO_PIN_RESET, DHT22_TIMEOUT_US) < 0) return -1;
    if (dht22_wait_level(dev, GPIO_PIN_SET, DHT22_TIMEOUT_US) < 0) return -1;

    for (int i = 0; i < 40; i++) {
        int high_us;
        if (dht22_wait_level(dev, GPIO_PIN_RESET, DHT22_TIMEOUT_US) < 0) return -1;
        high_us = dht22_wait_level(dev, GPIO_PIN_SET, DHT22_TIMEOUT_US);
        if (high_us < 0) return -1;
        data[i / 8] <<= 1;
        if ((uint32_t)high_us > DHT22_BIT_THRESHOLD) {
            data[i / 8] |= 1U;
        }
    }
    return dht22_decode_frame((const unsigned char *)data,
                              (short *)temp_x10,
                              (unsigned short *)humidity_x10);
}

int dht22_decode_frame(const unsigned char raw_frame[5], short *temp_x10,
                       unsigned short *humidity_x10)
{
    unsigned char sum;
    unsigned short raw_t;
    if (raw_frame == 0 || temp_x10 == 0 || humidity_x10 == 0) {
        return -1;
    }
    sum = (unsigned char)(raw_frame[0] + raw_frame[1] + raw_frame[2] + raw_frame[3]);
    if (sum != raw_frame[4]) {
        return -1;
    }
    *humidity_x10 = (unsigned short)(((unsigned short)raw_frame[0] << 8) | raw_frame[1]);
    raw_t = (unsigned short)(((unsigned short)raw_frame[2] << 8) | raw_frame[3]);
    *temp_x10 = (raw_t & 0x8000U) ? -(short)(raw_t & 0x7FFFU) : (short)raw_t;
    return 0;
}
