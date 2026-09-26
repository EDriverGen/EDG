#include "ssd1306.h"
#include <stdint.h>
#include <stddef.h>

#include <hal/hal_i2c.h>
#include <os/os_time.h>
static int ssd1306_write_cmd(struct ssd1306_dev *dev, uint8_t cmd) {
    struct hal_i2c_master_data pdata;
    pdata.address = dev->i2c_addr;
    pdata.buffer = &cmd;
    pdata.len = 1;
    return hal_i2c_master_write(0, &pdata, OS_TIMEOUT_NEVER, 1);
}

static int ssd1306_write_cmd_bytes(struct ssd1306_dev *dev, const uint8_t *bytes, size_t len) {
    struct hal_i2c_master_data pdata;
    pdata.address = dev->i2c_addr;
    pdata.buffer = (uint8_t *)bytes;
    pdata.len = len;
    return hal_i2c_master_write(0, &pdata, OS_TIMEOUT_NEVER, 1);
}

int ssd1306_init(struct ssd1306_dev *dev, void *bus_handle) {
    dev->bus_handle = bus_handle;
    dev->i2c_addr = SSD1306_I2C_ADDR;

    // Power ON sequence (simplified: assume RES# and VCC are handled externally)
    os_time_delay(100); // wait tAF
    ssd1306_write_cmd(dev, 0xAF);

    // Init sequence
    ssd1306_write_cmd(dev, 0xAE);
    uint8_t d5[] = {0xD5, 0x80};
    ssd1306_write_cmd_bytes(dev, d5, 2);
    uint8_t a8[] = {0xA8, 0x3F};
    ssd1306_write_cmd_bytes(dev, a8, 2);
    uint8_t d3[] = {0xD3, 0x00};
    ssd1306_write_cmd_bytes(dev, d3, 2);
    ssd1306_write_cmd(dev, 0x40);
    ssd1306_write_cmd(dev, 0xA1);
    ssd1306_write_cmd(dev, 0xC8);
    uint8_t da[] = {0xDA, 0x12};
    ssd1306_write_cmd_bytes(dev, da, 2);
    uint8_t ctrl[] = {0x81, 0x7F};
    ssd1306_write_cmd_bytes(dev, ctrl, 2);
    ssd1306_write_cmd(dev, 0xA4);
    ssd1306_write_cmd(dev, 0xA6);
    ssd1306_write_cmd(dev, 0x2E);
    uint8_t mode[] = {0x20, 0x00};
    ssd1306_write_cmd_bytes(dev, mode, 2);
    uint8_t col[] = {0x21, 0x00, 0x7F};
    ssd1306_write_cmd_bytes(dev, col, 3);
    uint8_t page[] = {0x22, 0x00, 0x07};
    ssd1306_write_cmd_bytes(dev, page, 3);
    ssd1306_write_cmd(dev, 0xAF);

    return 0;
}

int ssd1306_write_display_data(struct ssd1306_dev *dev, const uint8_t *data, size_t len) {
    // Set column address range (0x21, start, end)
    uint8_t col_cmd[] = {0x21, 0x00, 0x7F};
    ssd1306_write_cmd_bytes(dev, col_cmd, 3);
    // Set page address range (0x22, start, end)
    uint8_t page_cmd[] = {0x22, 0x00, 0x07};
    ssd1306_write_cmd_bytes(dev, page_cmd, 3);
    // Write data bytes (control byte with D/C#=1 is handled by HAL? Assume raw data write)
    struct hal_i2c_master_data pdata;
    pdata.address = dev->i2c_addr;
    pdata.buffer = (uint8_t *)data;
    pdata.len = len;
    return hal_i2c_master_write(0, &pdata, OS_TIMEOUT_NEVER, 1);
}
