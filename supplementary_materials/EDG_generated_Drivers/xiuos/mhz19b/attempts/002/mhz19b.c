#include "mhz19b.h"
#include <string.h>
#include <stdint.h>

#define CMD_READ_CO2 0x86
#define RESPONSE_LENGTH 9
#define START_BYTE 0xFF
#define COMMAND_BYTE 0x86

static int uart_write(int fd, const uint8_t *buf, size_t len) {
    return write(fd, buf, len);
}

static int uart_read(int fd, uint8_t *buf, size_t len) {
    return read(fd, buf, len);
}

static uint8_t compute_checksum(const uint8_t *cmd) {
    uint8_t sum = 0;
    for (int i = 1; i < 8; i++) {
        sum += cmd[i];
    }
    return ((~sum) + 1) & 0xFF;
}

int mhz19b_init(struct mhz19b_device *dev, void *bus_name) {
    const char *path = (const char *)bus_name;
    dev->fd = open(path, O_RDWR | O_NOCTTY);
    if (dev->fd < 0) {
        return -1;
    }
    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(dev->fd, &tty) != 0) {
        close(dev->fd);
        return -1;
    }
    cfsetospeed(&tty, B9600);
    cfsetispeed(&tty, B9600);
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag |= CREAD | CLOCAL;
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_iflag &= ~(INLCR | ICRNL | IGNCR);
    tty.c_oflag &= ~OPOST;
    tty.c_cc[VMIN] = 9;
    tty.c_cc[VTIME] = 10;
    if (tcsetattr(dev->fd, TCSANOW, &tty) != 0) {
        close(dev->fd);
        return -1;
    }
    return 0;
}

int mhz19b_read_co2(struct mhz19b_device *dev, int32_t *raw) {
    uint8_t cmd[9];
    cmd[0] = 0xFF;
    cmd[1] = 0x01;
    cmd[2] = CMD_READ_CO2;
    cmd[3] = 0x00;
    cmd[4] = 0x00;
    cmd[5] = 0x00;
    cmd[6] = 0x00;
    cmd[7] = 0x00;
    cmd[8] = compute_checksum(cmd);

    if (uart_write(dev->fd, cmd, 9) != 9) {
        return -1;
    }

    uint8_t resp[9];
    if (uart_read(dev->fd, resp, 9) != 9) {
        return -1;
    }

    if (resp[0] != START_BYTE || resp[1] != COMMAND_BYTE) {
        return -1;
    }

    uint8_t calc_checksum = ((~(resp[1] + resp[2] + resp[3] + resp[4] + resp[5] + resp[6] + resp[7])) + 1) & 0xFF;
    if (resp[8] != calc_checksum) {
        return -1;
    }

    *raw = (int32_t)((uint16_t)(resp[2] << 8) | resp[3]);
    return 0;
}