#include "dht22.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include <stddef.h>
#include <stdint.h>

#define DHT22_TRIG_PIN GPIO_PIN_0
#define DHT22_ECHO_PIN GPIO_PIN_1
#define DHT22_GPIO_PORT GPIOA

#define DHT22_TIMEOUT_US 1000

static void delay_us(uint32_t us) {
    uint32_t ticks = us * (SystemCoreClock / 1000000) / 2;
    for (uint32_t i = 0; i < ticks; i++) {
        __NOP();
    }
}

static int wait_for_pin_state(GPIO_TypeDef *port, uint16_t pin, GPIO_PinState state, uint32_t timeout_us) {
    while (timeout_us--) {
        if (HAL_GPIO_ReadPin(port, pin) == state) {
            return 0;
        }
        delay_us(1);
    }
    return -1;
}

int dht22_init(struct dht22_device *dev, void *bus_name) {
    (void)bus_name;
    dev->bus_handle = (void *)DHT22_GPIO_PORT;
    dev->trig_pin = DHT22_TRIG_PIN;
    dev->echo_pin = DHT22_ECHO_PIN;

    GPIO_InitTypeDef gpio_init = {0};
    gpio_init.Pin = DHT22_TRIG_PIN;
    gpio_init.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init.Pull = GPIO_NOPULL;
    gpio_init.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT22_GPIO_PORT, &gpio_init);

    gpio_init.Pin = DHT22_ECHO_PIN;
    gpio_init.Mode = GPIO_MODE_INPUT;
    gpio_init.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT22_GPIO_PORT, &gpio_init);

    HAL_GPIO_WritePin(DHT22_GPIO_PORT, DHT22_TRIG_PIN, GPIO_PIN_SET);
    HAL_Delay(1000);

    return 0;
}

int dht22_read_sensor(struct dht22_device *dev, int32_t *humidity_val, int32_t *temp_val) {
    if (!dev || !humidity_val || !temp_val) return -1;

    GPIO_TypeDef *port = (GPIO_TypeDef *)dev->bus_handle;
    uint16_t trig = dev->trig_pin;
    uint16_t echo = dev->echo_pin;

    // Send start signal: pull low for at least 18ms
    HAL_GPIO_WritePin(port, trig, GPIO_PIN_RESET);
    HAL_Delay(20);
    HAL_GPIO_WritePin(port, trig, GPIO_PIN_SET);
    delay_us(30);

    // Wait for sensor response: low 80us then high 80us
    if (wait_for_pin_state(port, echo, GPIO_PIN_RESET, DHT22_TIMEOUT_US) != 0) return -1;
    if (wait_for_pin_state(port, echo, GPIO_PIN_SET, DHT22_TIMEOUT_US) != 0) return -1;
    if (wait_for_pin_state(port, echo, GPIO_PIN_RESET, DHT22_TIMEOUT_US) != 0) return -1;

    // Read 40 bits
    uint8_t data[5] = {0};
    for (int i = 0; i < 40; i++) {
        // Wait for low (50us)
        if (wait_for_pin_state(port, echo, GPIO_PIN_SET, DHT22_TIMEOUT_US) != 0) return -1;
        // Measure high pulse width
        uint32_t high_ticks = 0;
        while (HAL_GPIO_ReadPin(port, echo) == GPIO_PIN_SET) {
            high_ticks++;
            delay_us(1);
            if (high_ticks > 100) break;
        }
        // Classify bit: if high pulse > 40us (approx) then bit=1 else 0
        if (high_ticks > 40) {
            data[i / 8] |= (1 << (7 - (i % 8)));
        }
    }

    // Verify checksum
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) return -1;

    // Convert to milli units
    uint16_t integral_rh = data[0];
    uint16_t decimal_rh = data[1];
    uint16_t integral_t = data[2];
    uint16_t decimal_t = data[3];

    *humidity_val = (int32_t)(integral_rh * 100 + decimal_rh);
    *temp_val = (int32_t)(integral_t * 100 + decimal_t);

    return 0;
}