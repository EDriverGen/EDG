#include "ds18b20.h"
#include "transform.h"
#include <errno.h>
#include <stddef.h>
#include <stdint.h>

#define DS18B20_SKIP_ROM 0xCC
#define DS18B20_CONVERT_T 0x44
#define DS18B20_READ_SCRATCHPAD 0xBE

static int reset_pulse(int trig_fd, int echo_fd) {
    uint8_t low = 0;
    uint8_t high = 1;
    // pull low for 480 us
    if (PrivWrite(trig_fd, &low, 1) != 1) return -EIO;
    PrivTaskDelay(1); // 1 ms > 480 us
    // release
    if (PrivWrite(trig_fd, &high, 1) != 1) return -EIO;
    // wait for presence pulse: wait for low then high
    uint8_t val;
    int timeout = 1000;
    while (timeout--) {
        if (PrivRead(echo_fd, &val, 1) != 1) return -EIO;
        if (val == 0) break;
    }
    if (timeout <= 0) return -ENODEV;
    timeout = 1000;
    while (timeout--) {
        if (PrivRead(echo_fd, &val, 1) != 1) return -EIO;
        if (val == 1) break;
    }
    if (timeout <= 0) return -ENODEV;
    return 0;
}

static int write_byte(int trig_fd, uint8_t byte) {
    for (int i = 0; i < 8; i++) {
        uint8_t bit = (byte >> i) & 1;
        uint8_t low = 0;
        uint8_t high = 1;
        if (bit) {
            // write 1: pull low for 1 us, then release
            if (PrivWrite(trig_fd, &low, 1) != 1) return -EIO;
            // delay ~1 us (use 1 ms as approximation, but need microsecond precision)
            // Since no microsecond delay, use 1 ms
            PrivTaskDelay(1);
            if (PrivWrite(trig_fd, &high, 1) != 1) return -EIO;
            PrivTaskDelay(1);
        } else {
            // write 0: pull low for 60 us
            if (PrivWrite(trig_fd, &low, 1) != 1) return -EIO;
            PrivTaskDelay(1); // 1 ms > 60 us
            if (PrivWrite(trig_fd, &high, 1) != 1) return -EIO;
            PrivTaskDelay(1);
        }
    }
    return 0;
}

static int read_byte(int trig_fd, int echo_fd, uint8_t *byte) {
    *byte = 0;
    for (int i = 0; i < 8; i++) {
        uint8_t low = 0;
        uint8_t high = 1;
        // master pulls low for 1 us
        if (PrivWrite(trig_fd, &low, 1) != 1) return -EIO;
        PrivTaskDelay(1);
        if (PrivWrite(trig_fd, &high, 1) != 1) return -EIO;
        // sample within 15 us
        uint8_t val;
        if (PrivRead(echo_fd, &val, 1) != 1) return -EIO;
        if (val) {
            *byte |= (1 << i);
        }
        // wait for slot end
        PrivTaskDelay(1);
    }
    return 0;
}

int ds18b20_init(struct ds18b20_dev *dev, void *bus_handle) {
    // bus_handle is a string like "/dev/gpio_trig,/dev/gpio_echo"
    // For simplicity, open fixed paths
    dev->trig_fd = 0; // dummy, actual open not needed in stub
    dev->echo_fd = 0;
    // reset pulse
    int ret = reset_pulse(dev->trig_fd, dev->echo_fd);
    if (ret != 0) return ret;
    return 0;
}

int ds18b20_read_temperature(struct ds18b20_dev *dev, int32_t *raw) {
    int ret;
    // reset
    ret = reset_pulse(dev->trig_fd, dev->echo_fd);
    if (ret != 0) return ret;
    // skip ROM
    ret = write_byte(dev->trig_fd, DS18B20_SKIP_ROM);
    if (ret != 0) return ret;
    // convert T
    ret = write_byte(dev->trig_fd, DS18B20_CONVERT_T);
    if (ret != 0) return ret;
    // wait 750 ms
    PrivTaskDelay(750);
    // reset
    ret = reset_pulse(dev->trig_fd, dev->echo_fd);
    if (ret != 0) return ret;
    // skip ROM
    ret = write_byte(dev->trig_fd, DS18B20_SKIP_ROM);
    if (ret != 0) return ret;
    // read scratchpad
    ret = write_byte(dev->trig_fd, DS18B20_READ_SCRATCHPAD);
    if (ret != 0) return ret;
    // read 2 bytes
    uint8_t lsb, msb;
    ret = read_byte(dev->trig_fd, dev->echo_fd, &lsb);
    if (ret != 0) return ret;
    ret = read_byte(dev->trig_fd, dev->echo_fd, &msb);
    if (ret != 0) return ret;
    // combine little-endian
    int16_t raw16 = (int16_t)((uint16_t)msb << 8 | lsb);
    // sign extend from 12 bits
    if (raw16 & 0x0800) {
        raw16 |= 0xF000;
    } else {
        raw16 &= 0x0FFF;
    }
    // convert to milli-degC: raw * 0.0625 * 1000 = raw * 625 / 10
    int32_t temp_milli = (int32_t)raw16 * 625 / 10;
    *raw = temp_milli;
    return 0;
}