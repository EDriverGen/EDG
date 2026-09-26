/*
 * MH-Z19B CO2 sensor driver for NuttX (UART)
 */
#include "mhz19b_ref.h"
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <string.h>

static uint8_t mhz19b_checksum(const uint8_t *data)
{
    uint8_t sum = 0;
    for (int i = 1; i < 8; i++) sum += data[i];
    return (~sum) + 1;
}

int mhz19b_init(struct mhz19b_device *dev, const char *uart_path)
{
    struct termios tty;
    if (!dev || !uart_path) return -EINVAL;
    dev->uart_fd = open(uart_path, O_RDWR | O_NOCTTY);
    if (dev->uart_fd < 0) return -errno;
    tcgetattr(dev->uart_fd, &tty);
    cfsetispeed(&tty, B9600); cfsetospeed(&tty, B9600);
    tty.c_cflag = CS8 | CREAD | CLOCAL;
    tty.c_iflag = 0; tty.c_oflag = 0; tty.c_lflag = 0;
    tty.c_cc[VMIN] = 9; tty.c_cc[VTIME] = 5;
    tcsetattr(dev->uart_fd, TCSANOW, &tty);
    return 0;
}

void mhz19b_deinit(struct mhz19b_device *dev)
{ if (dev && dev->uart_fd >= 0) { close(dev->uart_fd); dev->uart_fd = -1; } }

int mhz19b_read_co2(struct mhz19b_device *dev, uint16_t *ppm)
{
    uint8_t cmd[9] = {0}, resp[9] = {0};
    if (!dev || dev->uart_fd < 0 || !ppm) return -EINVAL;
    cmd[0] = MHZ19B_START_BYTE;
    cmd[1] = MHZ19B_SENSOR_NUM;
    cmd[2] = MHZ19B_CMD_READ_CO2;
    cmd[3] = 0; cmd[4] = 0; cmd[5] = 0; cmd[6] = 0; cmd[7] = 0;
    cmd[8] = mhz19b_checksum(cmd);
    if (write(dev->uart_fd, cmd, 9) != 9) return -EIO;
    usleep(100000);
    if (read(dev->uart_fd, resp, 9) != 9) return -EIO;
    if (resp[0] != MHZ19B_START_BYTE || resp[1] != MHZ19B_CMD_READ_CO2)
        return -EIO;
    if (resp[8] != mhz19b_checksum(resp))
        return -EIO;
    *ppm = (uint16_t)((uint16_t)resp[2] << 8 | resp[3]);
    return 0;
}

int mhz19b_set_abc(struct mhz19b_device *dev, uint8_t enable)
{
    uint8_t cmd[9] = {0};
    if (!dev || dev->uart_fd < 0) return -EINVAL;
    cmd[0] = MHZ19B_START_BYTE;
    cmd[1] = MHZ19B_SENSOR_NUM;
    cmd[2] = MHZ19B_CMD_ABC;
    cmd[3] = enable ? 0xA0 : 0x00;
    cmd[4] = 0; cmd[5] = 0; cmd[6] = 0; cmd[7] = 0;
    cmd[8] = mhz19b_checksum(cmd);
    return (write(dev->uart_fd, cmd, 9) == 9) ? 0 : -EIO;
}
