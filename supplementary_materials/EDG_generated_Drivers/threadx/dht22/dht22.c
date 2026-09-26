#include "dht22.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include <stddef.h>

#define DHT22_GPIO_PORT GPIOB
#define DHT22_DATA_PIN GPIO_PIN_5

static void delay_us(uint32_t us) {
    HAL_Delay(us);
}

static void delay_ms(uint32_t ms) {
    HAL_Delay(ms);
}

static void set_pin_output(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT22_DATA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT22_GPIO_PORT, &GPIO_InitStruct);
}

static void set_pin_input(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DHT22_DATA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(DHT22_GPIO_PORT, &GPIO_InitStruct);
}

static void pin_low(void) {
    HAL_GPIO_WritePin(DHT22_GPIO_PORT, DHT22_DATA_PIN, GPIO_PIN_RESET);
}

static void pin_high(void) {
    HAL_GPIO_WritePin(DHT22_GPIO_PORT, DHT22_DATA_PIN, GPIO_PIN_SET);
}

static int pin_read(void) {
    return (HAL_GPIO_ReadPin(DHT22_GPIO_PORT, DHT22_DATA_PIN) == GPIO_PIN_SET) ? 1 : 0;
}

static int wait_for_low(uint32_t timeout_us) {
    while (pin_read() && timeout_us > 0) {
        delay_us(1);
        timeout_us--;
    }
    return (timeout_us > 0) ? 0 : -1;
}

static int wait_for_high(uint32_t timeout_us) {
    while (!pin_read() && timeout_us > 0) {
        delay_us(1);
        timeout_us--;
    }
    return (timeout_us > 0) ? 0 : -1;
}

int dht22_init(struct dht22_device *dev, void *bus_handle) {
    if (!dev || !bus_handle) return -1;
    dev->bus_handle = bus_handle;
    delay_ms(1000);
    set_pin_output();
    pin_high();
    return 0;
}

int dht22_read_sensor(struct dht22_device *dev, int32_t *humidity_val, int32_t *temp_val) {
    if (!dev || !humidity_val || !temp_val) return -1;
    
    // Send start signal
    set_pin_output();
    pin_low();
    delay_ms(18);
    pin_high();
    delay_us(30);
    
    // Switch to input
    set_pin_input();
    
    // Wait for sensor response (low)
    if (wait_for_low(100) != 0) return -1;
    // Wait for response low duration (80us)
    delay_us(80);
    // Wait for response high (80us)
    if (wait_for_high(100) != 0) return -1;
    delay_us(80);
    
    // Read 40 bits
    uint8_t data[5] = {0};
    for (int i = 0; i < 40; i++) {
        // Wait for bit start low (50us)
        if (wait_for_low(100) != 0) return -1;
        delay_us(50);
        // Measure high pulse width
        if (wait_for_high(100) != 0) return -1;
        uint32_t high_start = 0;
        while (pin_read()) {
            delay_us(1);
            high_start++;
            if (high_start > 100) break;
        }
        // Classify bit: if high pulse > 40us (approx threshold) then 1 else 0
        if (high_start > 40) {
            data[i / 8] |= (1 << (7 - (i % 8)));
        }
    }
    
    // Verify checksum
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) return -1;
    
    uint16_t raw_hum = ((uint16_t)data[0] << 8) | data[1];
    uint16_t raw_temp = ((uint16_t)data[2] << 8) | data[3];
    int32_t temp_x10 = (raw_temp & 0x8000)
        ? -(int32_t)(raw_temp & 0x7FFF)
        : (int32_t)raw_temp;

    *humidity_val = (int32_t)raw_hum * 100;
    *temp_val = temp_x10 * 100;
    
    return 0;
}
