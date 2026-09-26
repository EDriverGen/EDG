#include "ssd1306.h"
#include "i2c_msg.h"
#include "hdf_base.h"
#include "osal_time.h"
#include <stdint.h>

#include "openharmony_liteosm.h"
static int ssd1306_write_cmd(struct ssd1306_dev *dev, uint8_t cmd)
{
    struct I2cMsg msg;
    uint8_t data[2];
    data[0] = 0x00; // control byte: Co=0, D/C#=0 (command)
    data[1] = cmd;
    msg.addr = dev->i2c_addr;
    msg.buf = data;
    msg.len = 2;
    msg.flags = 0;
    int32_t ret = I2cTransfer(dev->bus_handle, &msg, 1);
    return (ret == 1) ? 0 : -1;
}

static int ssd1306_write_cmd_with_arg(struct ssd1306_dev *dev, uint8_t cmd, uint8_t arg)
{
    struct I2cMsg msg;
    uint8_t data[3];
    data[0] = 0x00;
    data[1] = cmd;
    data[2] = arg;
    msg.addr = dev->i2c_addr;
    msg.buf = data;
    msg.len = 3;
    msg.flags = 0;
    int32_t ret = I2cTransfer(dev->bus_handle, &msg, 1);
    return (ret == 1) ? 0 : -1;
}

static int ssd1306_write_cmd_with_two_args(struct ssd1306_dev *dev, uint8_t cmd, uint8_t arg1, uint8_t arg2)
{
    struct I2cMsg msg;
    uint8_t data[4];
    data[0] = 0x00;
    data[1] = cmd;
    data[2] = arg1;
    data[3] = arg2;
    msg.addr = dev->i2c_addr;
    msg.buf = data;
    msg.len = 4;
    msg.flags = 0;
    int32_t ret = I2cTransfer(dev->bus_handle, &msg, 1);
    return (ret == 1) ? 0 : -1;
}

int ssd1306_init(struct ssd1306_dev *dev, DevHandle bus_handle)
{
    dev->bus_handle = bus_handle;
    dev->i2c_addr = SSD1306_I2C_ADDR;

    // init_display sequence
    if (ssd1306_write_cmd(dev, 0xAE) != 0) return -1;
    if (ssd1306_write_cmd_with_arg(dev, 0xD5, 0x80) != 0) return -1;
    if (ssd1306_write_cmd_with_arg(dev, 0xA8, 0x3F) != 0) return -1;
    if (ssd1306_write_cmd_with_arg(dev, 0xD3, 0x00) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0x40) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0xA1) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0xC8) != 0) return -1;
    if (ssd1306_write_cmd_with_arg(dev, 0xDA, 0x12) != 0) return -1;
    if (ssd1306_write_cmd_with_arg(dev, 0x81, 0x7F) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0xA4) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0xA6) != 0) return -1;
    if (ssd1306_write_cmd(dev, 0x2E) != 0) return -1;
    if (ssd1306_write_cmd_with_arg(dev, 0x20, 0x00) != 0) return -1;
    if (ssd1306_write_cmd_with_two_args(dev, 0x21, 0x00, 0x7F) != 0) return -1;
    if (ssd1306_write_cmd_with_two_args(dev, 0x22, 0x00, 0x07) != 0) return -1;

    // power_on: delay 100ms before AFh
    OsalMSleep(100);
    if (ssd1306_write_cmd(dev, 0xAF) != 0) return -1;

    return 0;
}

int ssd1306_write_display_data(struct ssd1306_dev *dev, const uint8_t *buf, uint16_t len)
{
    // Set column address range (0x21, start, end)
    if (ssd1306_write_cmd_with_two_args(dev, 0x21, 0x00, 0x7F) != 0) return -1;
    // Set page address range (0x22, start, end)
    if (ssd1306_write_cmd_with_two_args(dev, 0x22, 0x00, 0x07) != 0) return -1;

    // Write data: control byte with D/C#=1 (data)
    struct I2cMsg msg;
    uint8_t *data = (uint8_t *)OsalMemAlloc(sizeof(uint8_t) * (len + 1));
    if (data == NULL) return -1;
    data[0] = 0x40; // control byte: Co=0, D/C#=1 (data)
    for (uint16_t i = 0; i < len; i++) {
        data[i + 1] = buf[i];
    }
    msg.addr = dev->i2c_addr;
    msg.buf = data;
    msg.len = len + 1;
    msg.flags = 0;
    int32_t ret = I2cTransfer(dev->bus_handle, &msg, 1);
    OsalMemFree(data);
    return (ret == 1) ? 0 : -1;
}
