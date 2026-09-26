#include "ds18b20.h"
#include <errno.h>
#include <nuttx/clock.h>
#include <nuttx/ioexpander/gpio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "nuttx.h"
#define DS18B20_CMD_CONVERT_T     0x44
#define DS18B20_CMD_READ_SCRATCHPAD 0xBE

#define DS18B20_RESET_LOW_US      480
#define DS18B20_RESET_HIGH_US     480
#define DS18B20_PRESENCE_WAIT_US  70
#define DS18B20_PRESENCE_TIMEOUT_US 300

#define DS18B20_SLOT_DURATION_US  120
#define DS18B20_RECOVERY_US       1
#define DS18B20_WRITE_1_LOW_US    15
#define DS18B20_WRITE_0_LOW_US    120
#define DS18B20_READ_VALID_US     15

#define DS18B20_CONVERSION_DELAY_MS 750

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
    return ioctl(fd, GPIOC_SETPINTYPE, (unsigned long)1);
}

static int gpio_set_output(int fd)
{
    return ioctl(fd, GPIOC_SETPINTYPE, (unsigned long)0);
}

static void delay_us(unsigned int us)
{
    up_udelay(us);
}

static void delay_ms(unsigned int ms)
{
    up_mdelay(ms);
}

static int ds18b20_reset(struct ds18b20_dev *dev)
{
    int ret;
    bool presence;

    /* Set trig as output and pull low for reset pulse */
    ret = gpio_set_output(dev->trig_fd);
    if (ret < 0) return ret;
    ret = gpio_write(dev->trig_fd, 0);
    if (ret < 0) return ret;
    delay_us(DS18B20_RESET_LOW_US);

    /* Release line and set as input to detect presence */
    ret = gpio_set_input(dev->trig_fd);
    if (ret < 0) return ret;
    delay_us(DS18B20_PRESENCE_WAIT_US);

    /* Check for presence pulse (line pulled low by slave) */
    ret = gpio_read(dev->trig_fd, &presence);
    if (ret < 0) return ret;
    if (presence) {
        /* No presence detected */
        return -ENODEV;
    }

    /* Wait for presence pulse to end */
    unsigned int timeout = DS18B20_PRESENCE_TIMEOUT_US;
    while (timeout--) {
        ret = gpio_read(dev->trig_fd, &presence);
        if (ret < 0) return ret;
        if (presence) break;
        delay_us(1);
    }
    if (!presence) {
        return -ENODEV;
    }

    /* Wait for recovery */
    delay_us(DS18B20_RESET_HIGH_US - DS18B20_PRESENCE_WAIT_US);
    return 0;
}

static int ds18b20_write_bit(struct ds18b20_dev *dev, bool bit)
{
    int ret;
    ret = gpio_set_output(dev->trig_fd);
    if (ret < 0) return ret;
    ret = gpio_write(dev->trig_fd, 0);
    if (ret < 0) return ret;
    if (bit) {
        delay_us(DS18B20_WRITE_1_LOW_US);
        ret = gpio_write(dev->trig_fd, 1);
        if (ret < 0) return ret;
        delay_us(DS18B20_SLOT_DURATION_US - DS18B20_WRITE_1_LOW_US);
    } else {
        delay_us(DS18B20_WRITE_0_LOW_US);
        ret = gpio_write(dev->trig_fd, 1);
        if (ret < 0) return ret;
        delay_us(DS18B20_SLOT_DURATION_US - DS18B20_WRITE_0_LOW_US);
    }
    delay_us(DS18B20_RECOVERY_US);
    return 0;
}

static int ds18b20_read_bit(struct ds18b20_dev *dev, bool *bit)
{
    int ret;
    bool val;
    ret = gpio_set_output(dev->trig_fd);
    if (ret < 0) return ret;
    ret = gpio_write(dev->trig_fd, 0);
    if (ret < 0) return ret;
    delay_us(DS18B20_READ_VALID_US);
    ret = gpio_set_input(dev->trig_fd);
    if (ret < 0) return ret;
    ret = gpio_read(dev->trig_fd, &val);
    if (ret < 0) return ret;
    *bit = val;
    delay_us(DS18B20_SLOT_DURATION_US - DS18B20_READ_VALID_US);
    delay_us(DS18B20_RECOVERY_US);
    return 0;
}

static int ds18b20_write_byte(struct ds18b20_dev *dev, uint8_t byte)
{
    int ret;
    for (int i = 0; i < 8; i++) {
        ret = ds18b20_write_bit(dev, (byte >> i) & 1);
        if (ret < 0) return ret;
    }
    return 0;
}

static int ds18b20_read_byte(struct ds18b20_dev *dev, uint8_t *byte)
{
    int ret;
    uint8_t val = 0;
    bool bit;
    for (int i = 0; i < 8; i++) {
        ret = ds18b20_read_bit(dev, &bit);
        if (ret < 0) return ret;
        if (bit) val |= (1 << i);
    }
    *byte = val;
    return 0;
}

int ds18b20_init(struct ds18b20_dev *dev, const char *trig_path, const char *echo_path)
{
    int ret;

    /* Open GPIO devices */
    dev->trig_fd = open(trig_path, O_RDWR);
    if (dev->trig_fd < 0) {
        return -errno;
    }
    dev->echo_fd = open(echo_path, O_RDWR);
    if (dev->echo_fd < 0) {
        close(dev->trig_fd);
        return -errno;
    }

    /* Perform reset and presence detection */
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

    /* Skip ROM (assume single device) */
    ret = ds18b20_reset(dev);
    if (ret < 0) return ret;
    ret = ds18b20_write_byte(dev, 0xCC);
    if (ret < 0) return ret;
    ret = ds18b20_write_byte(dev, DS18B20_CMD_CONVERT_T);
    if (ret < 0) return ret;

    /* Wait for conversion */
    delay_ms(DS18B20_CONVERSION_DELAY_MS);

    /* Read scratchpad */
    ret = ds18b20_reset(dev);
    if (ret < 0) return ret;
    ret = ds18b20_write_byte(dev, 0xCC);
    if (ret < 0) return ret;
    ret = ds18b20_write_byte(dev, DS18B20_CMD_READ_SCRATCHPAD);
    if (ret < 0) return ret;

    ret = ds18b20_read_byte(dev, &lsb);
    if (ret < 0) return ret;
    ret = ds18b20_read_byte(dev, &msb);
    if (ret < 0) return ret;

    /* Combine bytes (little-endian) */
    raw_temp = (int16_t)((uint16_t)msb << 8 | lsb);

    /* Convert to milli-degrees Celsius: raw * 625 / 10 */
    *raw = ((int32_t)raw_temp * 625) / 10;

    return 0;
}
