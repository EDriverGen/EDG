#include "max31855_ref.h"

#include <dev/spi/spi.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

int max31855_init(struct max31855_device *dev, const char *spi_path)
{
    int fd;
    uint8_t mode = SPI_MODE_0;
    uint8_t bits = 8;
    uint32_t speed = MAX31855_SPI_MAX_HZ;

    if (dev == 0 || spi_path == 0) {
        return -1;
    }
    fd = open(spi_path, O_RDWR);
    if (fd < 0) {
        return -1;
    }
    (void)ioctl(fd, SPI_IOC_WR_MODE, &mode);
    (void)ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
    (void)ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
    (void)close(fd);
    dev->spi_path = spi_path;
    return 0;
}

int max31855_read_raw(struct max31855_device *dev, uint32_t *raw)
{
    struct spi_ioc_transfer msg;
    uint8_t rx[4];
    int fd;
    if (dev == 0 || dev->spi_path == 0 || raw == 0) {
        return -1;
    }
    fd = open(dev->spi_path, O_RDWR);
    if (fd < 0) {
        return -1;
    }
    msg.tx_buf = 0;
    msg.rx_buf = (uint64_t)(uintptr_t)rx;
    msg.len = 4;
    msg.speed_hz = MAX31855_SPI_MAX_HZ;
    msg.delay_usecs = 0;
    msg.bits_per_word = 8;
    msg.cs_change = 0;
    msg.mode = SPI_MODE_0;
    if (ioctl(fd, SPI_IOC_MESSAGE(1), &msg) != 0) {
        (void)close(fd);
        return -1;
    }
    (void)close(fd);
    *raw = ((uint32_t)rx[0] << 24) |
           ((uint32_t)rx[1] << 16) |
           ((uint32_t)rx[2] << 8) |
           rx[3];
    return 0;
}

int max31855_has_fault(uint32_t raw) { return (raw & MAX31855_FAULT_BIT) != 0; }
uint8_t max31855_get_fault(uint32_t raw) { return (uint8_t)(raw & (MAX31855_FAULT_SCV | MAX31855_FAULT_SCG | MAX31855_FAULT_OC)); }

int max31855_get_thermocouple_temp(uint32_t raw, int32_t *temp_mc)
{
    int32_t val;
    if (temp_mc == 0 || max31855_has_fault(raw)) {
        return -1;
    }
    val = (int32_t)(raw >> 18);
    if (val & 0x2000) {
        val |= ~((int32_t)0x3FFF);
    }
    *temp_mc = val * 250;
    return 0;
}

int max31855_get_internal_temp(uint32_t raw, int32_t *temp_mc)
{
    int32_t val;
    if (temp_mc == 0) {
        return -1;
    }
    val = (int32_t)((raw >> 4) & 0x0FFF);
    if (val & 0x0800) {
        val |= ~((int32_t)0x0FFF);
    }
    *temp_mc = (val * 625) / 10;
    return 0;
}

int max31855_read_thermocouple(struct max31855_device *dev, int32_t *temp_mc)
{
    uint32_t raw = 0;
    if (max31855_read_raw(dev, &raw) != 0) {
        return -1;
    }
    return max31855_get_thermocouple_temp(raw, temp_mc);
}

int max31855_read_internal(struct max31855_device *dev, int32_t *temp_mc)
{
    uint32_t raw = 0;
    if (max31855_read_raw(dev, &raw) != 0) {
        return -1;
    }
    return max31855_get_internal_temp(raw, temp_mc);
}
