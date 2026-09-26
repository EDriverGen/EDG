#include "ssd1306.h"
#include "i2c_msg.h"
#include "hdf_base.h"
#include "osal_time.h"
#include <stdint.h>

#include "openharmony_liteosm.h"
#define SSD1306_CMD 0x00
#define SSD1306_DATA 0x40

static int ssd1306_write_cmd(struct ssd1306_dev *dev, uint8_t cmd)
{
    struct I2cMsg msg;
    uint8_t buf[2];
    buf[0] = SSD1306_CMD;
    buf[1] = cmd;
    msg.addr = dev->i2c_addr;
    msg.buf = buf;
    msg.len = 2;
    msg.flags = 0;
    int32_t ret = I2cTransfer(dev->bus_handle, &msg, 1);
    return (ret == 1) ? 0 : -1;
}

static int ssd1306_write_cmd2(struct ssd1306_dev *dev, uint8_t cmd1, uint8_t cmd2)
{
    struct I2cMsg msg;
    uint8_t buf[3];
    buf[0] = SSD1306_CMD;
    buf[1] = cmd1;
    buf[2] = cmd2;
    msg.addr = dev->i2c_addr;
    msg.buf = buf;
    msg.len = 3;
    msg.flags = 0;
    int32_t ret = I2cTransfer(dev->bus_handle, &msg, 1);
    return (ret == 1) ? 0 : -1;
}

static int ssd1306_write_cmd3(struct ssd1306_dev *dev, uint8_t cmd1, uint8_t cmd2, uint8_t cmd3)
{
    struct I2cMsg msg;
    uint8_t buf[4];
    buf[0] = SSD1306_CMD;
    buf[1] = cmd1;
    buf[2] = cmd2;
    buf[3] = cmd3;
    msg.addr = dev->i2c_addr;
    msg.buf = buf;
    msg.len = 4;
    msg.flags = 0;
    int32_t ret = I2cTransfer(dev->bus_handle, &msg, 1);
    return (ret == 1) ? 0 : -1;
}

int ssd1306_init(struct ssd1306_dev *dev, DevHandle bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = SSD1306_I2C_ADDR;

    if (ssd1306_write_cmd(dev, 0xAE) != 0) return -1;
    if (ssd1306_write_cmd2(dev, 0xD5, 0x80) != 0) return -1;
    if (ssd1306_write_cmd2(dev, 0xA8, 0x3F) != 0) return -1;
    if (ssd1306_write_cmd2(dev, 0xD3, 0x00) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0x40) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0xA1) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0xC8) != 0) return -1;
    if (ssd1306_write_cmd2(dev, 0xDA, 0x12) != 0) return -1;
    if (ssd1306_write_cmd2(dev, 0x81, 0x7F) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0xA4) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0xA6) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0x2E) != 0) return -1;
    if (ssd1306_write_cmd2(dev, 0x20, 0x00) != 0) return -1;
    if (ssd1306_write_cmd3(dev, 0x21, 0x00, 0x7F) != 0) return -1;
    if (ssd1306_write_cmd3(dev, 0x22, 0x00, 0x07) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0xAF) != 0) return -1;

    return 0;
}

int ssd1306_write_display_data(struct ssd1306_dev *dev, const uint8_t *buf, uint32_t len)
{
    if (buf == NULL || len == 0) return -1;

    // Set column address range (0x21, start, end)
    if (ssd1306_write_cmd3(dev, 0x21, 0x00, 0x7F) != 0) return -1;
    // Set page address range (0x22, start, end)
    if (ssd1306_write_cmd3(dev, 0x22, 0x00, 0x07) != 0) return -1;

    // Write data: each I2C transfer sends control byte (0x40) followed by data bytes
    // To handle large data, we may need multiple transfers. For simplicity, send all in one transfer.
    struct I2cMsg msg;
    uint8_t *data_buf = (uint8_t *)buf; // We'll prepend control byte in a separate buffer
    // Since we cannot modify const buf, we create a new buffer with control byte
    uint32_t total_len = len + 1;
    uint8_t *tx_buf = (uint8_t *)0; // Placeholder, we'll allocate
    // For simplicity, assume len is small enough to fit in one transfer. Use stack buffer if len <= 128.
    // But to be safe, we'll use a static buffer or allocate. Since we don't have malloc, use a large stack buffer.
    // However, len could be up to 1024 (128*8). Use a loop sending chunks.
    // For this implementation, we'll send in chunks of up to 128 bytes.
    uint32_t offset = 0;
    while (offset < len) {
        uint32_t chunk = (len - offset > 128) ? 128 : (len - offset);
        uint8_t tx_buf[129];
        tx_buf[0] = SSD1306_DATA;
        for (uint32_t i = 0; i < chunk; i++) {
            tx_buf[i + 1] = buf[offset + i];
        }
        msg.addr = dev->i2c_addr;
        msg.buf = tx_buf;
        msg.len = chunk + 1;
        msg.flags = 0;
        int32_t ret = I2cTransfer(dev->bus_handle, &msg, 1);
        if (ret != 1) return -1;
        offset += chunk;
    }
    return 0;
}
