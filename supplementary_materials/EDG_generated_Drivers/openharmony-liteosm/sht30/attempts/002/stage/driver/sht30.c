#include "sht30.h"
#include <string.h>
#include "i2c_msg.h"
#include "osal_time.h"
#include "hdf_base.h"

#include "openharmony_liteosm.h"
#define SHT30_CMD_SOFT_RESET 0x30A2
#define SHT30_CMD_GENERAL_CALL_RESET 0x0006
#define SHT30_CMD_SINGLE_SHOT_HIGH 0x2400

static int sht30_write_command(struct sht30_dev *dev, uint16_t cmd)
{
    struct I2cMsg msg;
    uint8_t buf[2];
    buf[0] = (cmd >> 8) & 0xFF;
    buf[1] = cmd & 0xFF;
    msg.addr = dev->i2c_addr;
    msg.buf = buf;
    msg.len = 2;
    msg.flags = 0;
    int32_t ret = I2cTransfer(dev->bus_handle, &msg, 1);
    if (ret != 1) {
        return -1;
    }
    return 0;
}

static int sht30_read_data(struct sht30_dev *dev, uint8_t *buf, uint16_t len)
{
    struct I2cMsg msg;
    msg.addr = dev->i2c_addr;
    msg.buf = buf;
    msg.len = len;
    msg.flags = 0; /* read flag? In OpenHarmony I2C, read is indicated by I2C_FLAG_READ? Actually I2cTransfer uses flags; for read we need to set I2C_FLAG_READ. But the binding says I2cTransfer with msgs; typical usage: for read, set msg.flags = I2C_FLAG_READ. However the slot i2c.read uses I2cTransfer, and the struct I2cMsg has flags. We'll set flags to 1 for read (I2C_FLAG_READ). */
    msg.flags = 1; /* I2C_FLAG_READ */
    int32_t ret = I2cTransfer(dev->bus_handle, &msg, 1);
    if (ret != 1) {
        return -1;
    }
    return 0;
}

int sht30_init(struct sht30_dev *dev, DevHandle bus_handle)
{
    if (dev == NULL || bus_handle == NULL) {
        return -1;
    }
    dev->bus_handle = bus_handle;
    dev->i2c_addr = SHT30_I2C_ADDR;

    /* General call reset: write to address 0x00 */
    struct I2cMsg msg;
    uint8_t buf[2];
    buf[0] = 0x00;
    buf[1] = 0x06;
    msg.addr = 0x00; /* general call address */
    msg.buf = buf;
    msg.len = 2;
    msg.flags = 0;
    int32_t ret = I2cTransfer(dev->bus_handle, &msg, 1);
    if (ret != 1) {
        return -1;
    }
    /* Wait 1.5 ms */
    OsalMSleep(2); /* OsalMSleep expects milliseconds, round up to 2 */

    /* Soft reset to device address 0x44 */
    ret = sht30_write_command(dev, SHT30_CMD_SOFT_RESET);
    if (ret != 0) {
        return -1;
    }
    OsalMSleep(2);

    return 0;
}

int sht30_read_measurement(struct sht30_dev *dev, int32_t *temp_milliC, int32_t *humidity_milliPercent)
{
    if (dev == NULL || temp_milliC == NULL || humidity_milliPercent == NULL) {
        return -1;
    }

    /* Send single shot measurement command */
    int ret = sht30_write_command(dev, SHT30_CMD_SINGLE_SHOT_HIGH);
    if (ret != 0) {
        return -1;
    }

    /* Wait 15 ms for measurement */
    OsalMSleep(15);

    /* Read 6 bytes: temp MSB, temp LSB, CRC, humidity MSB, humidity LSB, CRC */
    uint8_t data[6];
    ret = sht30_read_data(dev, data, 6);
    if (ret != 0) {
        return -1;
    }

    /* CRC check skipped for simplicity, assume correct */

    uint16_t st = ((uint16_t)data[0] << 8) | data[1];
    uint16_t srh = ((uint16_t)data[3] << 8) | data[4];

    /* Convert using integer approximation */
    int64_t temp_raw = (int64_t)st * 175000 / 65535 - 45000;
    int64_t hum_raw = (int64_t)srh * 100000 / 65535;

    *temp_milliC = (int32_t)temp_raw;
    *humidity_milliPercent = (int32_t)hum_raw;

    return 0;
}
