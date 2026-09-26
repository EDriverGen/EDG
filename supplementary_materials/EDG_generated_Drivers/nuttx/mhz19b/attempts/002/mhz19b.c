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

int mhz19b_init(struct mhz19b_dev *dev, const char *bus_name)
{
    int fd = open(bus_name, O_RDWR | O_NONBLOCK);
    if (fd < 0) {
        return -1;
    }

    struct termios tio;
    memset(&tio, 0, sizeof(tio));
    cfsetospeed(&tio, B9600);
    cfsetispeed(&tio, B9600);
    tio.c_cflag = CS8 | CLOCAL | CREAD;
    tio.c_iflag = 0;
    tio.c_oflag = 0;
    tio.c_lflag = 0;
    tio.c_cc[VMIN] = 0;
    tio.c_cc[VTIME] = 10;

    if (tcsetattr(fd, TCSANOW, &tio) < 0) {
        close(fd);
        return -1;
    }

    dev->fd = fd;
    return 0;
}

int mhz19b_read_co2(struct mhz19b_dev *dev, int32_t *raw)
{
    uint8_t cmd[] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    uint8_t resp[MHZ19B_RESPONSE_LENGTH];

    if (uart_write(dev->fd, cmd, sizeof(cmd)) != 0) {
        return -1;
    }

    up_mdelay(100);

    if (uart_read(dev->fd, resp, MHZ19B_RESPONSE_LENGTH) != 0) {
        return -1;
    }

    if (resp[0] != 0xFF || resp[1] != 0x86) {
        return -1;
    }

    uint8_t checksum = 0;
    for (int i = 1; i < 8; i++) {
        checksum += resp[i];
    }
    checksum = (~checksum) + 1;
    if (checksum != resp[8]) {
        return -1;
    }

    *raw = (int32_t)((uint16_t)resp[2] * 256 + (uint16_t)resp[3]);
    return 0;
}
