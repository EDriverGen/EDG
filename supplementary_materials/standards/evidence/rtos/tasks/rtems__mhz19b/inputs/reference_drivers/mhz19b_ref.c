#include "mhz19b_ref.h"

#include <fcntl.h>
#include <unistd.h>

static uint8_t mhz19b_checksum(const uint8_t *data)
{
    uint8_t sum = 0;
    for (int i = 1; i < 8; i++) {
        sum = (uint8_t)(sum + data[i]);
    }
    return (uint8_t)((uint8_t)(~sum) + 1U);
}

static int mhz19b_write_frame(const char *path, const uint8_t frame[MHZ19B_FRAME_LEN])
{
    int fd = open(path, O_RDWR);
    ssize_t n;
    if (fd < 0) {
        return -1;
    }
    n = write(fd, frame, MHZ19B_FRAME_LEN);
    (void)close(fd);
    return n == (ssize_t)MHZ19B_FRAME_LEN ? 0 : -1;
}

static int mhz19b_read_frame(const char *path, uint8_t frame[MHZ19B_FRAME_LEN])
{
    int fd = open(path, O_RDWR);
    ssize_t n;
    if (fd < 0) {
        return -1;
    }
    n = read(fd, frame, MHZ19B_FRAME_LEN);
    (void)close(fd);
    return n == (ssize_t)MHZ19B_FRAME_LEN ? 0 : -1;
}

int mhz19b_init(struct mhz19b_device *dev, const char *uart_path)
{
    if (dev == 0 || uart_path == 0) {
        return -1;
    }
    dev->uart_path = uart_path;
    return 0;
}

int mhz19b_read_co2(struct mhz19b_device *dev, uint16_t *ppm)
{
    uint8_t cmd[MHZ19B_FRAME_LEN] = {0};
    uint8_t resp[MHZ19B_FRAME_LEN] = {0};
    if (dev == 0 || dev->uart_path == 0 || ppm == 0) {
        return -1;
    }
    cmd[0] = MHZ19B_START_BYTE;
    cmd[1] = MHZ19B_SENSOR_NUM;
    cmd[2] = MHZ19B_CMD_READ_CO2;
    cmd[8] = mhz19b_checksum(cmd);
    if (mhz19b_write_frame(dev->uart_path, cmd) != 0 ||
        mhz19b_read_frame(dev->uart_path, resp) != 0) {
        return -1;
    }
    if (resp[0] != MHZ19B_START_BYTE || resp[1] != MHZ19B_CMD_READ_CO2) {
        return -1;
    }
    if (resp[8] != mhz19b_checksum(resp)) {
        return -1;
    }
    *ppm = (uint16_t)(((uint16_t)resp[2] << 8) | resp[3]);
    return 0;
}

int mhz19b_set_abc(struct mhz19b_device *dev, uint8_t enable)
{
    uint8_t cmd[MHZ19B_FRAME_LEN] = {0};
    if (dev == 0 || dev->uart_path == 0) {
        return -1;
    }
    cmd[0] = MHZ19B_START_BYTE;
    cmd[1] = MHZ19B_SENSOR_NUM;
    cmd[2] = MHZ19B_CMD_ABC;
    cmd[3] = enable ? 0xA0U : 0x00U;
    cmd[8] = mhz19b_checksum(cmd);
    return mhz19b_write_frame(dev->uart_path, cmd);
}

int mhz19b_calibrate_zero(struct mhz19b_device *dev)
{
    uint8_t cmd[MHZ19B_FRAME_LEN] = {0};
    if (dev == 0 || dev->uart_path == 0) {
        return -1;
    }
    cmd[0] = MHZ19B_START_BYTE;
    cmd[1] = MHZ19B_SENSOR_NUM;
    cmd[2] = MHZ19B_CMD_CALIBRATE;
    cmd[8] = mhz19b_checksum(cmd);
    return mhz19b_write_frame(dev->uart_path, cmd);
}
