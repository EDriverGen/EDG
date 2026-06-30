/* OpenHarmony HDF spi_if.h stub */
#ifndef __SPI_IF_STUB_H
#define __SPI_IF_STUB_H

#include "openharmony_liteosm.h"

/* SPI message structure */
struct SpiMsg {
    uint8_t *wbuf;
    uint8_t *rbuf;
    uint32_t len;
    uint32_t speed;
    uint16_t delayUs;
    uint8_t csChange;
    uint8_t keepCs;
};

/* SPI configuration */
struct SpiCfg {
    uint32_t maxSpeedHz;
    uint16_t mode;
    uint8_t  transferMode;
    uint8_t  bitsPerWord;
};

#define SPI_CLK_PHASE   (1 << 0)
#define SPI_CLK_POLARITY (1 << 1)
#define SPI_MODE_0       0
#define SPI_MODE_1       SPI_CLK_PHASE
#define SPI_MODE_2       SPI_CLK_POLARITY
#define SPI_MODE_3       (SPI_CLK_PHASE | SPI_CLK_POLARITY)
#define SPI_MODE_CS_HIGH (1 << 2)
#define SPI_MODE_LSB_FIRST (1 << 3)
#define SPI_MODE_3WIRE   (1 << 4)
#define SPI_MODE_LOOP    (1 << 5)

/* SPI transfer mode */
#define SPI_POLLING_TRANSFER  0
#define SPI_DMA_TRANSFER      1

/* SPI device info for SpiOpen */
struct SpiDevInfo {
    uint32_t busNum;
    uint32_t csNum;
};

DevHandle SpiOpen(struct SpiDevInfo *info);
void SpiClose(DevHandle handle);
int32_t SpiTransfer(DevHandle handle, struct SpiMsg *msgs, uint32_t count);
int32_t SpiRead(DevHandle handle, uint8_t *buf, uint32_t len);
int32_t SpiWrite(DevHandle handle, uint8_t *buf, uint32_t len);
int32_t SpiSetCfg(DevHandle handle, struct SpiCfg *cfg);
int32_t SpiGetCfg(DevHandle handle, struct SpiCfg *cfg);

#endif /* __SPI_IF_STUB_H */
