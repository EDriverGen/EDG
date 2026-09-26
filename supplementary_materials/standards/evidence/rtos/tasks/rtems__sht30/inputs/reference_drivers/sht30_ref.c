#include "sht30_ref.h"

#include <dev/i2c/i2c.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

static uint8_t sht30_crc8(const uint8_t *data, uint16_t len)
{
    uint8_t crc = 0xFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t bit = 0; bit < 8; bit++) {
            crc = (crc & 0x80U) ? (uint8_t)((crc << 1) ^ 0x31U) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

static int sht30_transfer(struct sht30_device *dev, struct i2c_msg *msgs,
                          uint32_t nmsgs)
{
    struct i2c_rdwr_ioctl_data rdwr;
    int fd;
    int ret;
    if (dev == 0 || dev->bus_path == 0 || msgs == 0 || nmsgs == 0) {
        return -1;
    }
    rdwr.msgs = msgs;
    rdwr.nmsgs = nmsgs;
    fd = open(dev->bus_path, O_RDWR);
    if (fd < 0) {
        return -1;
    }
    ret = ioctl(fd, I2C_RDWR, &rdwr);
    (void)close(fd);
    return ret == 0 ? 0 : -1;
}

static int sht30_write_command(struct sht30_device *dev, const uint8_t cmd[2])
{
    struct i2c_msg msg;
    msg.addr = dev->addr;
    msg.flags = 0;
    msg.len = 2;
    msg.buf = (uint8_t *)cmd;
    return sht30_transfer(dev, &msg, 1);
}

int sht30_init(struct sht30_device *dev, const char *bus_path, uint16_t addr)
{
    if (dev == 0 || bus_path == 0 || (addr != SHT30_ADDR_DEFAULT && addr != SHT30_ADDR_ALT)) {
        return -1;
    }
    dev->bus_path = bus_path;
    dev->addr = addr;
    return 0;
}

int sht30_probe(struct sht30_device *dev)
{
    static const uint8_t soft_reset[2] = {0x30, 0xA2};
    return sht30_write_command(dev, soft_reset);
}

int sht30_read(struct sht30_device *dev, int32_t *temp_mcelsius, int32_t *rh_mpercent)
{
    static const uint8_t measure[2] = {0x24, 0x00};
    struct i2c_msg msg;
    uint8_t data[6];
    uint16_t raw_temp;
    uint16_t raw_hum;

    if (dev == 0 || temp_mcelsius == 0 || rh_mpercent == 0) {
        return -1;
    }
    if (sht30_write_command(dev, measure) != 0) {
        return -1;
    }
    (void)rtems_task_wake_after(RTEMS_MILLISECONDS_TO_TICKS(20));
    msg.addr = dev->addr;
    msg.flags = I2C_M_RD;
    msg.len = sizeof(data);
    msg.buf = data;
    if (sht30_transfer(dev, &msg, 1) != 0) {
        return -1;
    }
    if (sht30_crc8(&data[0], 2) != data[2] || sht30_crc8(&data[3], 2) != data[5]) {
        return -2;
    }

    raw_temp = (uint16_t)(((uint16_t)data[0] << 8) | data[1]);
    raw_hum = (uint16_t)(((uint16_t)data[3] << 8) | data[4]);
    *temp_mcelsius = -45000 + (int32_t)((uint64_t)raw_temp * 175000ULL / 65535ULL);
    *rh_mpercent = (int32_t)((uint64_t)raw_hum * 100000ULL / 65535ULL);
    return 0;
}
