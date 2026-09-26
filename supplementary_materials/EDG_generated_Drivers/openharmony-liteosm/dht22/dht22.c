#include "dht22.h"
#include "gpio_if.h"
#include "osal_time.h"
#include "hdf_base.h"
#include <stdint.h>

#define DHT22_GPIO_DIR_OUT 0
#define DHT22_GPIO_DIR_IN 1
#define DHT22_GPIO_VAL_LOW 0
#define DHT22_GPIO_VAL_HIGH 1

static int32_t gpio_write_pin(uint16_t gpio, uint16_t val)
{
    return GpioWrite(gpio, val);
}

static int32_t gpio_read_pin(uint16_t gpio, uint16_t *val)
{
    return GpioRead(gpio, val);
}

static int32_t gpio_set_dir(uint16_t gpio, uint16_t dir)
{
    return GpioSetDir(gpio, dir);
}

int32_t dht22_init(struct dht22_device *dev, uint16_t gpio_pin)
{
    dev->gpio_pin = gpio_pin;
    int32_t ret;
    ret = gpio_set_dir(dev->gpio_pin, DHT22_GPIO_DIR_OUT);
    if (ret != HDF_SUCCESS) {
        return -1;
    }
    ret = gpio_write_pin(dev->gpio_pin, DHT22_GPIO_VAL_HIGH);
    if (ret != HDF_SUCCESS) {
        return -1;
    }
    ret = gpio_set_dir(dev->gpio_pin, DHT22_GPIO_DIR_IN);
    if (ret != HDF_SUCCESS) {
        return -1;
    }
    OsalMSleep(1000);
    return 0;
}

static int32_t wait_for_level(struct dht22_device *dev, uint16_t target, uint32_t timeout_us)
{
    uint32_t elapsed = 0;
    uint16_t val;
    while (elapsed < timeout_us) {
        if (gpio_read_pin(dev->gpio_pin, &val) != HDF_SUCCESS) {
            return -1;
        }
        if (val == target) {
            return 0;
        }
        OsalUDelay(1);
        elapsed++;
    }
    return -1;
}

int32_t dht22_read_sensor(struct dht22_device *dev, int32_t *humidity_val, int32_t *temp_val)
{
    if (humidity_val == NULL || temp_val == NULL) {
        return -1;
    }
    int32_t ret;
    uint16_t val;
    uint8_t data[5] = {0};
    int bit_index;
    int byte_index;
    
    // Send start signal: pull low for at least 18ms
    ret = gpio_set_dir(dev->gpio_pin, DHT22_GPIO_DIR_OUT);
    if (ret != HDF_SUCCESS) return -1;
    ret = gpio_write_pin(dev->gpio_pin, DHT22_GPIO_VAL_LOW);
    if (ret != HDF_SUCCESS) return -1;
    OsalMSleep(20); // 20ms > 18ms
    
    // Pull high and wait for sensor response
    ret = gpio_write_pin(dev->gpio_pin, DHT22_GPIO_VAL_HIGH);
    if (ret != HDF_SUCCESS) return -1;
    OsalUDelay(30); // wait 20-40us
    
    // Set to input to release bus
    ret = gpio_set_dir(dev->gpio_pin, DHT22_GPIO_DIR_IN);
    if (ret != HDF_SUCCESS) return -1;
    
    // Wait for sensor to pull low (response low)
    if (wait_for_level(dev, DHT22_GPIO_VAL_LOW, 200) != 0) {
        return -1; // -EIO replaced with -1
    }
    // Wait for sensor to pull high (response high)
    if (wait_for_level(dev, DHT22_GPIO_VAL_HIGH, 200) != 0) {
        return -1;
    }
    // Wait for sensor to pull low again (start of data)
    if (wait_for_level(dev, DHT22_GPIO_VAL_LOW, 200) != 0) {
        return -1;
    }
    
    // Read 40 bits
    for (bit_index = 0; bit_index < 40; bit_index++) {
        // Wait for high pulse
        if (wait_for_level(dev, DHT22_GPIO_VAL_HIGH, 100) != 0) {
            return -1;
        }
        // Measure high pulse width
        uint32_t high_start = 0;
        uint32_t high_count = 0;
        while (high_count < 100) {
            if (gpio_read_pin(dev->gpio_pin, &val) != HDF_SUCCESS) {
                return -1;
            }
            if (val == DHT22_GPIO_VAL_HIGH) {
                high_count++;
            } else {
                break;
            }
            OsalUDelay(1);
        }
        // Determine bit: if high_count > 40 (approx 40us threshold), bit=1 else bit=0
        uint8_t bit = (high_count > 40) ? 1 : 0;
        byte_index = bit_index / 8;
        data[byte_index] = (data[byte_index] << 1) | bit;
        
        // Wait for low to start next bit (if not last bit)
        if (bit_index < 39) {
            if (wait_for_level(dev, DHT22_GPIO_VAL_LOW, 100) != 0) {
                return -1;
            }
        }
    }
    
    // Verify checksum
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4]) {
        return -1;
    }
    
    uint16_t raw_hum = ((uint16_t)data[0] << 8) | data[1];
    uint16_t raw_temp = ((uint16_t)data[2] << 8) | data[3];
    int32_t temp_x10 = (raw_temp & 0x8000)
        ? -(int32_t)(raw_temp & 0x7FFF)
        : (int32_t)raw_temp;

    *humidity_val = (int32_t)raw_hum * 100;
    *temp_val = temp_x10 * 100;
    
    return 0;
}
