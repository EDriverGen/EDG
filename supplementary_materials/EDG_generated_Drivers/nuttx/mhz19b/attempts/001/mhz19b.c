#include "mhz19b.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include "arch.h"

#include "nuttx.h"
#define MHZ19B_CMD_READ_CO2 0x86
#define MHZ19B_RESPONSE_LENGTH 9

static uint8_t mhz19b_checksum(const uint8_t *data, int len)
{
    uint8_t sum = 0;
    for (int i = 0; i < len; i++) {
        sum += data[i];
    }
    return ((~sum) + 1) & 0xFF;
}

int mhz19b_init(struct mhz19b_dev *dev, const char *bus_name)
{
    if (!dev || !bus_name) {
        return -EINVAL;
    }
    int fd = open(bus_name, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        return -errno;
    }
    struct termios tio;
    memset(&tio, 0, sizeof(tio));
    cfsetospeed(&tio, B9600);
    cfsetispeed(&tio, B9600);
    tio.c_cflag = CS8 | CLOCAL | CREAD;
    tio.c_iflag = 0;
    tio.c_oflag = 0;
    tio.c_lflag = 0;
    tio.c_cc[VMIN] = 9;
    tio.c_cc[VTIME] = 10;
    tcflush(fd, TCIOFLUSH);
    tcsetattr(fd, TCSANOW, &tio);
    dev->fd = fd;
    return 0;
}

int mhz19b_read_co2(struct mhz19b_dev *dev, int32_t *raw)
{
    if (!dev || !raw) {
        return -EINVAL;
    }
    uint8_t cmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    int ret = write(dev->fd, cmd, sizeof(cmd));
    if (ret != sizeof(cmd)) {
        return -EIO;
    }
    up_mdelay(100);
    uint8_t resp[MHZ19B_RESPONSE_LENGTH];
    ret = read(dev->fd, resp, MHZ19B_RESPONSE_LENGTH);
    if (ret != MHZ19B_RESPONSE_LENGTH) {
        return -EIO;
    }
    if (resp[0] != 0xFF) {
        return -EIO;
    }
    if (resp[1] != 0x86) {
        return -EIO;
    }
    uint8_t calc_cs = mhz19b_checksum(resp + 1, 7);
    if (calc_cs != resp[8]) {
        return -EIO;
    }
    *raw = (int32_t)((uint16_t)resp[2] * 256 + (uint16_t)resp[3]);
    return 0;
}
