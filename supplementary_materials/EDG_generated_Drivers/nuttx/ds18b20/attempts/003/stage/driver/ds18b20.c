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
#define DS18B20_PRESENCE_WAIT_US  480
#define DS18B20_SLOT_US           120
#define DS18B20_RECOVERY_US       1
#define DS18B20_READ_DATA_VALID_US 15
#define DS18B20_CONVERSION_MS     750

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

static int ds18b20_reset_pulse(struct ds18b20_dev *dev)
{
    int ret;
    bool presence;

    /* Set trig as output and pull low for reset pulse */
    ret = gpio_set_output(dev->trig_fd);
    if (ret < 0) return ret;
    ret = gpio_write(dev->trig_fd, 0);
    if (ret < 0) return ret;
    up_mdelay(1); /* at least 480 us, use 1 ms for simplicity */

    /* Release line and wait for presence */
    ret = gpio_set_input(dev->trig_fd);
    if (ret < 0) return ret;
    up_mdelay(1); /* wait for presence pulse */

    /* Check presence: read echo pin (should be low) */
    ret = gpio_read(dev->echo_fd, &presence);
    if (ret < 0) return ret;
    if (presence) {
        return -ENODEV;
    }

    /* Wait for presence pulse to end */
    up_mdelay(1);
    return 0;
}

static int ds18b20_write_bit(struct ds18b20_dev *dev, int bit)
{
    int ret;
    ret = gpio_set_output(dev->trig_fd);
    if (ret < 0) return ret;
    ret = gpio_write(dev->trig_fd, 0);
    if (ret < 0) return ret;
    if (bit) {
        up_mdelay(1); /* write 1 slot: low for ~15 us, then high */
        ret = gpio_write(dev->trig_fd, 1);
        if (ret < 0) return ret;
        up_mdelay(1);
    } else {
        up_mdelay(1); /* write 0 slot: low for ~120 us */
        ret = gpio_write(dev->trig_fd, 1);
        if (ret < 0) return ret;
        up_mdelay(1);
    }
    return 0;
}

static int ds18b20_read_bit(struct ds18b20_dev *dev, int *bit)
{
    int ret;
    bool val;
    ret = gpio_set_output(dev->trig_fd);
    if (ret < 0) return ret;
    ret = gpio_write(dev->trig_fd, 0);
    if (ret < 0) return ret;
    up_mdelay(1); /* hold low for ~1 us */
    ret = gpio_set_input(dev->trig_fd);
    if (ret < 0) return ret;
    up_mdelay(1); /* wait for data valid */
    ret = gpio_read(dev->echo_fd, &val);
    if (ret < 0) return ret;
    *bit = val ? 1 : 0;
    up_mdelay(1); /* complete slot */
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
    int bit;
    uint8_t val = 0;
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

    dev->trig_fd = open(trig_path, O_RDWR);
    if (dev->trig_fd < 0) return -errno;
    dev->echo_fd = open(echo_path, O_RDWR);
    if (dev->echo_fd < 0) {
        close(dev->trig_fd);
        return -errno;
    }

    ret = ds18b20_reset_pulse(dev);
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
    ret = ds18b20_write_byte(dev, 0xCC);
    if (ret < 0) return ret;

    /* Convert T */
    ret = ds18b20_write_byte(dev, DS18B20_CMD_CONVERT_T);
    if (ret < 0) return ret;

    /* Wait for conversion */
    up_mdelay(DS18B20_CONVERSION_MS);

    /* Reset */
    ret = ds18b20_reset_pulse(dev);
    if (ret < 0) return ret;

    /* Skip ROM again */
    ret = ds18b20_write_byte(dev, 0xCC);
    if (ret < 0) return ret;

    /* Read scratchpad */
    ret = ds18b20_write_byte(dev, DS18B20_CMD_READ_SCRATCHPAD);
    if (ret < 0) return ret;

    ret = ds18b20_read_byte(dev, &lsb);
    if (ret < 0) return ret;
    ret = ds18b20_read_byte(dev, &msb);
    if (ret < 0) return ret;

    raw_temp = (int16_t)((msb << 8) | lsb);
    /* Sign extend from 12 bits */
    if (raw_temp & 0x0800) {
        raw_temp |= 0xF000;
    } else {
        raw_temp &= 0x0FFF;
    }

    /* Convert to milli degrees: raw * 625 / 10 */
    *raw = ((int32_t)raw_temp * 625) / 10;

    return 0;
}
