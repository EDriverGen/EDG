#ifndef DRIVERGEN_RTEMS_DEV_SPI_SPI_H
#define DRIVERGEN_RTEMS_DEV_SPI_SPI_H

#include "linux/spi/spidev.h"
#include "rtems.h"

typedef struct spi_ioc_transfer spi_ioc_transfer;

#define SPI_BUS_OBTAIN  0x6B20u
#define SPI_BUS_RELEASE 0x6B21u

#endif
