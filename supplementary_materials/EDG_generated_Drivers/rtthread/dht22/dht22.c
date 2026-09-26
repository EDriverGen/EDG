#include "dht22.h"
#include "rtthread.h"
#include <stdint.h>
#include <errno.h>

#define DHT22_PIN_MODE_OUTPUT 0
#define DHT22_PIN_MODE_INPUT 1
#define DHT22_PIN_HIGH 1
#define DHT22_PIN_LOW 0

static void delay_us(uint32_t us)
{
    volatile uint32_t i;
    for (i = 0; i < us * 10; i++) {
        __asm__ volatile ("nop");
    }
}

int dht22_init(struct dht22_device *dev, uint64_t pin)
{
    dev->pin = pin;
    rt_pin_mode(pin, DHT22_PIN_MODE_OUTPUT);
    rt_pin_write(pin, DHT22_PIN_HIGH);
    rt_thread_mdelay(1000);
    return 0;
}

static int dht22_read_bit(struct dht22_device *dev)
{
    uint32_t timeout = 0;
    while (rt_pin_read(dev->pin) == 0) {
        if (++timeout > 100) return -EIO;
        delay_us(1);
    }
    timeout = 0;
    while (rt_pin_read(dev->pin) == 1) {
        if (++timeout > 100) return -EIO;
        delay_us(1);
    }
    return (timeout > 30) ? 1 : 0;
}

int dht22_read_sensor(struct dht22_device *dev, int32_t *humidity, int32_t *temperature)
{
    uint8_t data[5] = {0};
    int i, j;
    uint32_t timeout;

    rt_pin_mode(dev->pin, DHT22_PIN_MODE_OUTPUT);
    rt_pin_write(dev->pin, DHT22_PIN_LOW);
    rt_thread_mdelay(20);
    rt_pin_write(dev->pin, DHT22_PIN_HIGH);
    delay_us(30);
    rt_pin_mode(dev->pin, DHT22_PIN_MODE_INPUT);

    timeout = 0;
    while (rt_pin_read(dev->pin) == 1) {
        if (++timeout > 100) return -EIO;
        delay_us(1);
    }
    timeout = 0;
    while (rt_pin_read(dev->pin) == 0) {
        if (++timeout > 200) return -EIO;
        delay_us(1);
    }
    timeout = 0;
    while (rt_pin_read(dev->pin) == 1) {
        if (++timeout > 200) return -EIO;
        delay_us(1);
    }

    for (i = 0; i < 5; i++) {
        for (j = 7; j >= 0; j--) {
            timeout = 0;
            while (rt_pin_read(dev->pin) == 0) {
                if (++timeout > 100) return -EIO;
                delay_us(1);
            }
            timeout = 0;
            while (rt_pin_read(dev->pin) == 1) {
                if (++timeout > 100) return -EIO;
                delay_us(1);
            }
            if (timeout > 30) {
                data[i] |= (1 << j);
            }
        }
    }

    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) {
        return -EIO;
    }

    uint16_t raw_hum = ((uint16_t)data[0] << 8) | data[1];
    uint16_t raw_temp = ((uint16_t)data[2] << 8) | data[3];
    int32_t temp_x10 = (raw_temp & 0x8000)
        ? -(int32_t)(raw_temp & 0x7FFF)
        : (int32_t)raw_temp;

    *humidity = (int32_t)raw_hum * 100;
    *temperature = temp_x10 * 100;

    return 0;
}
