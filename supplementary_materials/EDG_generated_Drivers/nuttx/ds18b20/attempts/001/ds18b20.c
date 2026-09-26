#include "ds18b20.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <nuttx/ioexpander/gpio.h>
#include <nuttx/clock.h>

#include "nuttx.h"
#define DS18B20_CONVERT_T 0x44
#define DS18B20_READ_SCRATCHPAD 0xBE
#define DS18B20_SKIP_ROM 0xCC

#define RESET_LOW_US 480
#define RESET_HIGH_US 480
#define PRESENCE_WAIT_US 60
#define PRESENCE_TIMEOUT_US 240
#define CONVERSION_DELAY_MS 750
#define SLOT_DURATION_US 120
#define RECOVERY_US 1

static int gpio_write(int fd, bool value)
{
    return ioctl(fd, GPIOC_WRITE, (unsigned long)value);
}

static int gpio_read(int fd, bool *value)
{
    return ioctl(fd, GPIOC_READ, (unsigned long)value);
}

static int gpio_set_input(int fd)
{
    return ioctl(fd, GPIOC_SETPINTYPE, (unsigned long)0);
}

static int gpio_set_output(int fd)
{
    return ioctl(fd, GPIOC_SETPINTYPE, (unsigned long)1);
}

static void delay_us(unsigned int us)
{
    up_mdelay((us + 999) / 1000);
}

static int ds18b20_reset(struct ds18b20_dev *dev)
{
    bool presence;
    int ret;

    /* Set trig as output and pull low for reset pulse */
    ret = gpio_set_output(dev->trig_fd);
    if (ret < 0) return ret;
    ret = gpio_write(dev->trig_fd, 0);
    if (ret < 0) return ret;
    up_mdelay(1); /* at least 480 us, use 1 ms for simplicity */

    /* Release line and set as input to detect presence */
    ret = gpio_set_input(dev->trig_fd);
    if (ret < 0) return ret;
    up_mdelay(1); /* wait for presence pulse */

    /* Read presence pulse */
    ret = gpio_read(dev->trig_fd, &presence);
    if (ret < 0) return ret;
    if (presence) {
        return -ENODEV;
    }
    return 0;
}

static int ds18b20_write_byte(struct ds18b20_dev *dev, uint8_t byte)
{
    int ret;
    for (int i = 0; i < 8; i++) {
        bool bit = (byte >> i) & 1;
        ret = gpio_set_output(dev->trig_fd);
        if (ret < 0) return ret;
        ret = gpio_write(dev->trig_fd, 0);
        if (ret < 0) return ret;
        if (bit) {
            delay_us(15);
        } else {
            delay_us(120);
        }
        ret = gpio_write(dev->trig_fd, 1);
        if (ret < 0) return ret;
        delay_us(1);
    }
    return 0;
}

static int ds18b20_read_byte(struct ds18b20_dev *dev, uint8_t *byte)
{
    int ret;
    uint8_t result = 0;
    for (int i = 0; i < 8; i++) {
        bool bit;
        ret = gpio_set_output(dev->trig_fd);
        if (ret < 0) return ret;
        ret = gpio_write(dev->trig_fd, 0);
        if (ret < 0) return ret;
        delay_us(1);
        ret = gpio_set_input(dev->trig_fd);
        if (ret < 0) return ret;
        delay_us(15);
        ret = gpio_read(dev->trig_fd, &bit);
        if (ret < 0) return ret;
        if (bit) {
            result |= (1 << i);
        }
        delay_us(120);
    }
    *byte = result;
    return 0;
}

int ds18b20_init(struct ds18b20_dev *dev, const char *trig_path, const char *echo_path)
{
    int ret;

    dev->trig_fd = open(trig_path, O_RDWR);
    if (dev->trig_fd < 0) {
        return -errno;
    }
    dev->echo_fd = open(echo_path, O_RDWR);
    if (dev->echo_fd < 0) {
        close(dev->trig_fd);
        return -errno;
    }

    ret = ds18b20_reset(dev);
    if (ret < 0) {
        close(dev->trig_fd);
        close(dev->echo_fd);
        return ret;
    }

    return 0;
}

int ds18b20_read_temperature(struct ds18b20_dev *dev, int32_t *raw)
{
    int ret;
    uint8_t lsb, msb;
    int16_t raw_temp;

    /* Skip ROM */
    ret = ds18b20_write_byte(dev, DS18B20_SKIP_ROM);
    if (ret < 0) return ret;

    /* Convert T */
    ret = ds18b20_write_byte(dev, DS18B20_CONVERT_T);
    if (ret < 0) return ret;

    /* Wait for conversion */
    up_mdelay(CONVERSION_DELAY_MS);

    /* Skip ROM again */
    ret = ds18b20_write_byte(dev, DS18B20_SKIP_ROM);
    if (ret < 0) return ret;

    /* Read scratchpad */
    ret = ds18b20_write_byte(dev, DS18B20_READ_SCRATCHPAD);
    if (ret < 0) return ret;

    ret = ds18b20_read_byte(dev, &lsb);
    if (ret < 0) return ret;
    ret = ds18b20_read_byte(dev, &msb);
    if (ret < 0) return ret;

    raw_temp = (int16_t)((msb << 8) | lsb);
    /* Sign extend from 12 bits */
    if (raw_temp & 0x800) {
        raw_temp |= 0xF000;
    }

    /* Convert to milli degrees: raw * 625 / 10 */
    *raw = ((int32_t)raw_temp * 625) / 10;

    return 0;
}
