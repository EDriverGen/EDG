#include "dht22.h"
#include <nuttx/ioexpander/gpio.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>
#include <stdbool.h>
#include "arch.h"

#define DHT22_START_SIGNAL_LOW_MS 1
#define DHT22_RESPONSE_WAIT_US 30
#define DHT22_RESPONSE_LOW_US 80
#define DHT22_RESPONSE_HIGH_US 80
#define DHT22_BIT_LOW_US 50
#define DHT22_BIT_0_HIGH_US 27
#define DHT22_BIT_1_HIGH_US 70
#define DHT22_TIMEOUT_US 200

static int gpio_write(int fd, bool value)
{
    return ioctl(fd, GPIOC_WRITE, (unsigned long)value);
}

static int gpio_read(int fd, bool *value)
{
    return ioctl(fd, GPIOC_READ, (unsigned long)value);
}

static int gpio_set_dir(int fd, int pintype)
{
    return ioctl(fd, GPIOC_SETPINTYPE, (unsigned long)pintype);
}

static int wait_for_level(int fd, bool target, int timeout_us)
{
    bool level;
    int elapsed = 0;
    while (elapsed < timeout_us) {
        if (gpio_read(fd, &level) < 0)
            return -EIO;
        if (level == target)
            return 0;
        up_udelay(1);
        elapsed += 1;
    }
    return -ETIMEDOUT;
}

static int measure_pulse_high(int fd, int timeout_us)
{
    bool level;
    int width = 0;
    while (width < timeout_us) {
        if (gpio_read(fd, &level) < 0)
            return -EIO;
        if (!level)
            break;
        up_udelay(1);
        width++;
    }
    return width;
}

int dht22_init(struct dht22_dev *dev, const char *trig_path, const char *echo_path)
{
    dev->trig_fd = open(trig_path, O_RDWR);
    if (dev->trig_fd < 0)
        return -errno;
    dev->echo_fd = open(echo_path, O_RDWR);
    if (dev->echo_fd < 0) {
        close(dev->trig_fd);
        return -errno;
    }
    /* Configure echo pin as input initially */
    if (gpio_set_dir(dev->echo_fd, GPIO_INPUT_PIN) < 0) {
        close(dev->trig_fd);
        close(dev->echo_fd);
        return -EIO;
    }
    /* Power-up delay */
    up_mdelay(1000);
    return 0;
}

int dht22_read_sensor(struct dht22_dev *dev, int32_t *humidity_val, int32_t *temp_val)
{
    int ret;
    bool level;
    uint8_t data[5] = {0};
    int bit_idx;
    int byte_idx;
    int pulse_width;
    
    /* Send start signal: pull low for at least 18ms */
    if (gpio_set_dir(dev->echo_fd, GPIO_OUTPUT_PIN) < 0)
        return -EIO;
    if (gpio_write(dev->echo_fd, false) < 0)
        return -EIO;
    up_mdelay(DHT22_START_SIGNAL_LOW_MS);
    
    /* Release line and switch to input */
    if (gpio_write(dev->echo_fd, true) < 0)
        return -EIO;
    if (gpio_set_dir(dev->echo_fd, GPIO_INPUT_PIN) < 0)
        return -EIO;
    
    /* Wait for sensor response: pull low */
    ret = wait_for_level(dev->echo_fd, false, DHT22_RESPONSE_WAIT_US + DHT22_RESPONSE_LOW_US + DHT22_RESPONSE_HIGH_US);
    if (ret < 0)
        return -EIO;
    
    /* Wait for response low duration */
    ret = wait_for_level(dev->echo_fd, true, DHT22_RESPONSE_LOW_US + DHT22_TIMEOUT_US);
    if (ret < 0)
        return -EIO;
    
    /* Wait for response high duration */
    ret = wait_for_level(dev->echo_fd, false, DHT22_RESPONSE_HIGH_US + DHT22_TIMEOUT_US);
    if (ret < 0)
        return -EIO;
    
    /* Read 40 bits. Each bit: LOW 50us → HIGH 27us(0) or 70us(1).
     * We detect the rising edge, measure the HIGH pulse, classify. */
    for (bit_idx = 0; bit_idx < 40; bit_idx++) {
        /* Wait for rising edge (end of LOW phase, start of data pulse) */
        ret = wait_for_level(dev->echo_fd, true, DHT22_BIT_1_HIGH_US + DHT22_TIMEOUT_US);
        if (ret < 0)
            return -EIO;
        /* Measure HIGH pulse width NOW (before it falls back LOW) */
        pulse_width = measure_pulse_high(dev->echo_fd, DHT22_BIT_1_HIGH_US + DHT22_TIMEOUT_US);
        if (pulse_width < 0)
            return -EIO;
        /* 27us=0, 70us=1; threshold at ~50us midpoint */
        if (pulse_width > 50) {
            data[bit_idx / 8] |= (1 << (7 - (bit_idx % 8)));
        }
    }
    
    /* Verify checksum */
    uint8_t sum = data[0] + data[1] + data[2] + data[3];
    if (sum != data[4])
        return -EIO;
    
    /* DHT22 data format: MSB-first, value ×10.  Byte order:
     *   data[0]=humidity_hi, data[1]=humidity_lo,
     *   data[2]=temp_hi, data[3]=temp_lo, data[4]=checksum.
     * Output unit: milli_percent_rh / milli_degC → ×100 from raw. */
    int32_t raw_humidity = ((int32_t)data[0] << 8) | data[1];
    int32_t raw_temp     = ((int32_t)data[2] << 8) | data[3];

    /* Handle negative temperature: bit 15 set → value = -(raw & 0x7FFF) */
    if (raw_temp & 0x8000) {
        raw_temp = -(raw_temp & 0x7FFF);
    }

    *humidity_val = raw_humidity * 100;
    *temp_val     = raw_temp * 100;
    
    return 0;
}