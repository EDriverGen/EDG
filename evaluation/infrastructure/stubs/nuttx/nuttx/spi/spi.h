/* NuttX nuttx/spi/spi.h stub */
#ifndef __NUTTX_SPI_STUB_H
#define __NUTTX_SPI_STUB_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifndef FAR
#define FAR
#endif

/* SPI mode flags */
#define SPIDEV_MODE0     0
#define SPIDEV_MODE1     1
#define SPIDEV_MODE2     2
#define SPIDEV_MODE3     3

/* SPI bus frequency */
#define SPI_SPEED_100KHZ   100000
#define SPI_SPEED_400KHZ   400000
#define SPI_SPEED_1MHZ    1000000
#define SPI_SPEED_10MHZ  10000000

/* SPI device types for chip select */
#define SPIDEV_FLASH(n)       ((n) + 0x100)
#define SPIDEV_DISPLAY(n)     ((n) + 0x200)
#define SPIDEV_ACCELEROMETER(n) ((n) + 0x300)
#define SPIDEV_ADC(n)         ((n) + 0x400)
#define SPIDEV_TEMPERATURE(n) ((n) + 0x500)
#define SPIDEV_USER(n)        ((n) + 0x600)
#define SPIDEV_BAROMETER(n)   ((n) + 0x700)

/* SPI data width */
#define SPI_NBITS_8    8
#define SPI_NBITS_16  16

/* SPI device struct (opaque lower-half driver) */
struct spi_dev_s {
    void *priv;
};

/* SPI operations — function-based API.
 *
 * We always enable CONFIG_SPI_EXCHANGE in this evaluation stub because the
 * companion stubs_spi.c provides a real full-duplex SPI_EXCHANGE function
 * (routed through hw_spi1_xfer_byte, one CS-bracketed frame).
 *
 * Leaving CONFIG_SPI_EXCHANGE undefined would activate a fallback macro
 * that rewrites every SPI_EXCHANGE into
 *   SPI_SNDBLOCK(tx, n); SPI_RECVBLOCK(rx, n);
 * which is HALF-duplex and breaks any device whose MISO data arrives during
 * the master's command bytes (MCP3008, MAX31855, shift-register ADCs, ...).
 */
#ifndef CONFIG_SPI_EXCHANGE
#define CONFIG_SPI_EXCHANGE 1
#endif

void SPI_LOCK(FAR struct spi_dev_s *dev, bool lock);
void SPI_SELECT(FAR struct spi_dev_s *dev, uint32_t devid, bool selected);
uint32_t SPI_SETFREQUENCY(FAR struct spi_dev_s *dev, uint32_t frequency);
void SPI_SETMODE(FAR struct spi_dev_s *dev, int mode);
void SPI_SETBITS(FAR struct spi_dev_s *dev, int nbits);
uint16_t SPI_SEND(FAR struct spi_dev_s *dev, uint16_t wd);
void SPI_EXCHANGE(FAR struct spi_dev_s *dev, FAR const void *txbuf,
                  FAR void *rxbuf, size_t nwords);
void SPI_SNDBLOCK(FAR struct spi_dev_s *dev, FAR const void *buf, size_t nwords);
void SPI_RECVBLOCK(FAR struct spi_dev_s *dev, FAR void *buf, size_t nwords);

/* Board-level SPI bus initialization */
FAR struct spi_dev_s *board_spibus_initialize(int bus);

#endif /* __NUTTX_SPI_STUB_H__ */
