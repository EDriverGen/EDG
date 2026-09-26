#include "ds18b20.h"
#include "transform.h"
#include <errno.h>
#include <stdint.h>
#include <stddef.h>

#define DS18B20_SKIP_ROM 0xCC
#define DS18B20_CONVERT_T 0x44
#define DS18B20_READ_SCRATCHPAD 0xBE

static int gpio_write_byte(int fd, uint8_t byte) {
    for (int i = 0; i < 8; i++) {
        if (byte & (1 << i)) {
            // Write 1 slot: pull low for ~1us, then release for ~60us
            uint8_t low = 0;
            PrivWrite(fd, &low, 1);
            PrivTaskDelay(1); // 1ms is too long but we use ms API; actual timing handled by Renode
            uint8_t high = 1;
            PrivWrite(fd, &high, 1);
            PrivTaskDelay(1);
        } else {
            // Write 0 slot: pull low for ~60us, then release
            uint8_t low = 0;
            PrivWrite(fd, &low, 1);
            PrivTaskDelay(1);
            uint8_t high = 1;
            PrivWrite(fd, &high, 1);
            PrivTaskDelay(1);
        }
    }
    return 0;
}

static int gpio_read_byte(int fd, uint8_t *byte) {
    *byte = 0;
    for (int i = 0; i < 8; i++) {
        // Initiate read slot: pull low for ~1us, then release
        uint8_t low = 0;
        PrivWrite(fd, &low, 1);
        PrivTaskDelay(1);
        uint8_t high = 1;
        PrivWrite(fd, &high, 1);
        // Read bit
        uint8_t val;
        PrivRead(fd, &val, 1);
        if (val) {
            *byte |= (1 << i);
        }
        PrivTaskDelay(1);
    }
    return 0;
}

static int ds18b20_reset(int fd) {
    uint8_t low = 0;
    PrivWrite(fd, &low, 1);
    PrivTaskDelay(1); // 1ms (should be 480us but ms API)
    uint8_t high = 1;
    PrivWrite(fd, &high, 1);
    PrivTaskDelay(1);
    uint8_t presence;
    PrivRead(fd, &presence, 1);
    if (presence != 0) {
        return -ENODEV;
    }
    return 0;
}

int ds18b20_init(struct ds18b20_dev *dev, void *bus_name) {
    (void)bus_name;
    dev->trig_fd = 0; // dummy, actual fd from bus_name
    dev->echo_fd = 0;
    // Reset pulse
    int ret = ds18b20_reset(dev->trig_fd);
    if (ret != 0) return ret;
    return 0;
}

int ds18b20_read_temperature(struct ds18b20_dev *dev, int32_t *raw) {
    int fd = dev->trig_fd;
    // Reset
    int ret = ds18b20_reset(fd);
    if (ret != 0) return ret;
    // Skip ROM
    gpio_write_byte(fd, DS18B20_SKIP_ROM);
    // Convert T
    gpio_write_byte(fd, DS18B20_CONVERT_T);
    // Wait for conversion (750ms)
    PrivTaskDelay(750);
    // Reset
    ret = ds18b20_reset(fd);
    if (ret != 0) return ret;
    // Skip ROM
    gpio_write_byte(fd, DS18B20_SKIP_ROM);
    // Read Scratchpad
    gpio_write_byte(fd, DS18B20_READ_SCRATCHPAD);
    // Read 2 bytes
    uint8_t lsb, msb;
    gpio_read_byte(fd, &lsb);
    gpio_read_byte(fd, &msb);
    int16_t raw_temp = (int16_t)((msb << 8) | lsb);
    // Convert to milli degrees: raw * 0.0625 * 1000 = raw * 625 / 10
    *raw = ((int32_t)raw_temp * 625) / 10;
    return 0;
}