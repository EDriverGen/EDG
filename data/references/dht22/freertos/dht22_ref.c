/*
 * DHT22 sensor driver for FreeRTOS + STM32 HAL
 */
#include "dht22_ref.h"

static void dht22_set_output(struct dht22_device *dev)
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = dev->data_pin; gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(dev->data_port, &gpio);
}

static void dht22_set_input(struct dht22_device *dev)
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = dev->data_pin; gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(dev->data_port, &gpio);
}

static int dht22_wait(struct dht22_device *dev, GPIO_PinState level, int max_us)
{
    int us = 0;
    while (HAL_GPIO_ReadPin(dev->data_port, dev->data_pin) == level) {
        dht22_delay_us(1);
        if (++us > max_us) return -1;
    }
    return us;
}

int dht22_init(struct dht22_device *dev, GPIO_TypeDef *port, uint16_t pin)
{
    if (!dev || !port) return -1;
    dev->data_port = port; dev->data_pin = pin;
    dht22_set_output(dev);
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
    return 0;
}

int dht22_read(struct dht22_device *dev, int16_t *temp_x10, uint16_t *humidity_x10)
{
    uint8_t data[5] = {0};
    if (!dev || !temp_x10 || !humidity_x10) return -1;

    /* start signal */
    dht22_set_output(dev);
    HAL_GPIO_WritePin(dev->data_port, dev->data_pin, GPIO_PIN_RESET);
    HAL_Delay(2);
    HAL_GPIO_WritePin(dev->data_port, dev->data_pin, GPIO_PIN_SET);
    dht22_delay_us(30);
    dht22_set_input(dev);

    /* sensor response */
    if (dht22_wait(dev, GPIO_PIN_RESET, DHT22_TIMEOUT_US) < 0) return -1;
    if (dht22_wait(dev, GPIO_PIN_SET, DHT22_TIMEOUT_US) < 0) return -1;

    /* read 40 bits */
    for (int i = 0; i < 40; i++) {
        if (dht22_wait(dev, GPIO_PIN_RESET, DHT22_TIMEOUT_US) < 0) return -1;
        int high_us = dht22_wait(dev, GPIO_PIN_SET, DHT22_TIMEOUT_US);
        if (high_us < 0) return -1;
        data[i / 8] <<= 1;
        if (high_us > DHT22_BIT_THRESHOLD) data[i / 8] |= 1;
    }

    /* verify checksum */
    uint8_t sum = (uint8_t)(data[0] + data[1] + data[2] + data[3]);
    if (sum != data[4]) return -1;
    /* humidity: data[0..1], 0.1% RH */
    *humidity_x10 = (uint16_t)((uint16_t)data[0] << 8 | data[1]);
    /* temperature: data[2..3], 0.1 degC, bit15=sign */
    uint16_t raw_t = (uint16_t)((uint16_t)data[2] << 8 | data[3]);
    if (raw_t & 0x8000)
        *temp_x10 = -(int16_t)(raw_t & 0x7FFF);
    else
        *temp_x10 = (int16_t)raw_t;
    return 0;
}
