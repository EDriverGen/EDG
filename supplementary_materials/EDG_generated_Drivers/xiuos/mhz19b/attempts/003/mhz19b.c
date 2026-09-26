#include "mhz19b.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#define MHZ19B_CMD_READ_CO2 0x86
#define MHZ19B_RESPONSE_LENGTH 9

static uint8_t mhz19b_checksum(const uint8_t *buf, int len) {
    uint8_t sum = 0;
    for (int i = 0; i < len; i++) {
        sum += buf[i];
    }
    return ((~sum) + 1) & 0xFF;
}

int mhz19b_init(struct mhz19b_device *dev, void *bus_name) {
    const char *path = (const char *)bus_name;
    if (path == NULL) {
        path = "/dev/uart1";
    }
    int fd = open(path, O_RDWR | O_NOCTTY);
    if (fd < 0) {
        return -1;
    }
    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(fd, &tty) != 0) {
        close(fd);
        return -1;
    }
    cfsetospeed(&tty, B9600);
    cfsetispeed(&tty, B9600);
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag |= CREAD | CLOCAL;
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(INLCR | ICRNL | IGNCR);
    tty.c_oflag &= ~OPOST;
    tty.c_cc[VMIN] = 9;
    tty.c_cc[VTIME] = 10;
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        close(fd);
        return -1;
    }
    dev->fd = fd;
    return 0;
}

int mhz19b_read_co2(struct mhz19b_device *dev, int32_t *raw) {
    uint8_t cmd[9] = {0xFF, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};
    int ret = write(dev->fd, cmd, 9);
    if (ret != 9) {
        return -1;
    }
    usleep(100000);
    uint8_t resp[9];
    ret = read(dev->fd, resp, 9);
    if (ret != 9) {
        return -1;
    }
    if (resp[0] != 0xFF || resp[1] != 0x86) {
        return -1;
    }
    uint8_t calc_checksum = mhz19b_checksum(resp, 8);
    if (calc_checksum != resp[8]) {
        return -1;
    }
    *raw = (int32_t)((uint16_t)resp[2] * 256 + (uint16_t)resp[3]);
    return 0;
}