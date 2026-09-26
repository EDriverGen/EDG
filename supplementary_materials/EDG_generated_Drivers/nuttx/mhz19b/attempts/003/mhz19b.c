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

static int uart_write(int fd, const uint8_t *buf, size_t len)
{
    ssize_t ret = write(fd, buf, len);
    if (ret < 0 || (size_t)ret != len) {
        return -1;
    }
    return 0;
}

static int uart_read(int fd, uint8_t *buf, size_t len)
{
    size_t total = 0;
    while (total < len) {
        ssize_t ret = read(fd, buf + total, len - total);
        if (ret <= 0) {
            return -1;
        }
        total += ret;
    }
    return 0;
}

static uint8_t compute_checksum(const uint8_t *cmd, size_t len)
{
    uint8_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += cmd[i];
    }
    return (~sum) + 1;
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
    tio.c_iflag = IGNPAR;
    tio.c_oflag = 0;
    tio.c_lflag = 0;
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 0;
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &tio);
    dev->fd = fd;
    return 0;
}

int mhz19b_read_co2(struct mhz19b_dev *dev, int32_t *raw)
{
    if (!dev || !raw) {
        return -EINVAL;
    }
    uint8_t cmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    cmd[8] = compute_checksum(cmd, 8);
    if (uart_write(dev->fd, cmd, 9) != 0) {
        return -EIO;
    }
    up_mdelay(100);
    uint8_t resp[9];
    if (uart_read(dev->fd, resp, 9) != 0) {
        return -EIO;
    }
    if (resp[0] != 0xFF) {
        return -EIO;
    }
    if (resp[1] != 0x86) {
        return -EIO;
    }
    uint8_t calc_checksum = compute_checksum(resp, 8);
    if (calc_checksum != resp[8]) {
        return -EIO;
    }
    *raw = (int32_t)((uint16_t)resp[2] << 8 | resp[3]);
    return 0;
}
