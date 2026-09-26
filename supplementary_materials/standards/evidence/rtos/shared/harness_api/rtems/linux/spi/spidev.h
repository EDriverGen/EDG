#ifndef DRIVERGEN_RTEMS_LINUX_SPI_SPIDEV_H
#define DRIVERGEN_RTEMS_LINUX_SPI_SPIDEV_H

#include <stdint.h>

#define SPI_CPHA      0x01
#define SPI_CPOL      0x02
#define SPI_MODE_0    0x00
#define SPI_MODE_1    SPI_CPHA
#define SPI_MODE_2    SPI_CPOL
#define SPI_MODE_3    (SPI_CPOL | SPI_CPHA)
#define SPI_CS_HIGH   0x04
#define SPI_LSB_FIRST 0x08

struct spi_ioc_transfer {
    uint64_t tx_buf;
    uint64_t rx_buf;
    uint32_t len;
    uint32_t speed_hz;
    uint16_t delay_usecs;
    uint8_t bits_per_word;
    uint8_t cs_change;
    uint32_t mode;
};

#define SPI_IOC_MAGIC 's'
#define SPI_IOC_MESSAGE(n)       (0x6C00u | ((n) & 0xFFu))
#define SPI_IOC_RD_MODE          0x6B01u
#define SPI_IOC_WR_MODE          0x6B02u
#define SPI_IOC_RD_BITS_PER_WORD 0x6B03u
#define SPI_IOC_WR_BITS_PER_WORD 0x6B04u
#define SPI_IOC_RD_MAX_SPEED_HZ  0x6B05u
#define SPI_IOC_WR_MAX_SPEED_HZ  0x6B06u

#endif
