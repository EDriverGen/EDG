#include "adxl345.h"
#include "ztimer.h"
#include <stdint.h>
#include <stddef.h>

#include "riot.h"
#define ADXL345_REG_POWER_CTL 0x2D
#define ADXL345_REG_DATAX0    0x32
#define ADXL345_READ_CMD      0x80
#define ADXL345_MB            0x40

int adxl345_init(adxl345_t *dev, spi_t bus, spi_cs_t cs)
{
    dev->bus = bus;
    dev->cs = cs;

    spi_init_cs(bus, cs);

    uint8_t tx_buf[2];
    uint8_t rx_buf[2];

    // Write POWER_CTL = 0x00 (standby)
    tx_buf[0] = ADXL345_REG_POWER_CTL;
    tx_buf[1] = 0x00;
    spi_acquire(bus, cs, SPI_MODE_0, SPI_CLK_5MHZ);
    spi_transfer_bytes(bus, cs, false, tx_buf, rx_buf, 2);
    spi_release(bus);

    // Write POWER_CTL = 0x08 (measurement mode)
    tx_buf[0] = ADXL345_REG_POWER_CTL;
    tx_buf[1] = 0x08;
    spi_acquire(bus, cs, SPI_MODE_0, SPI_CLK_5MHZ);
    spi_transfer_bytes(bus, cs, false, tx_buf, rx_buf, 2);
    spi_release(bus);

    // Wait for turn-on time (11.1 ms)
    ztimer_sleep(ZTIMER_MSEC, 12);

    return 0;
}

int adxl345_read_xyz(adxl345_t *dev, int16_t *ax, int16_t *ay, int16_t *az)
{
    uint8_t cmd = ADXL345_REG_DATAX0 | ADXL345_READ_CMD | ADXL345_MB;
    uint8_t tx_buf[7];
    uint8_t rx_buf[7];
    tx_buf[0] = cmd;
    for (int i = 1; i < 7; i++) {
        tx_buf[i] = 0;
    }

    spi_acquire(dev->bus, dev->cs, SPI_MODE_0, SPI_CLK_5MHZ);
    spi_transfer_bytes(dev->bus, dev->cs, false, tx_buf, rx_buf, 7);
    spi_release(dev->bus);

    // rx_buf[0] is echo of command, data starts at rx_buf[1]
    uint8_t *data = &rx_buf[1];
    int16_t x = (int16_t)(data[0] | (data[1] << 8));
    int16_t y = (int16_t)(data[2] | (data[3] << 8));
    int16_t z = (int16_t)(data[4] | (data[5] << 8));

    *ax = x;
    *ay = y;
    *az = z;

    return 0;
}
