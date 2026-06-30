/* W25Q64JV SPI NOR Flash for ChibiOS/HAL. */
#ifndef REF_W25Q64JV_CHIBIOS_H_
#define REF_W25Q64JV_CHIBIOS_H_
#include <stdint.h>
#include "hal.h"
#define W25Q64JV_PAGE_SIZE 256
#define W25Q64JV_SECTOR_SIZE 4096
#define W25Q64JV_SIZE 8388608
#define W25Q64_CMD_WRITE_ENABLE  0x06
#define W25Q64_CMD_READ_SR1      0x05
#define W25Q64_CMD_READ_DATA     0x03
#define W25Q64_CMD_PAGE_PROGRAM  0x02
#define W25Q64_CMD_SECTOR_ERASE  0x20
#define W25Q64_CMD_JEDEC_ID      0x9F
#define W25Q64_SR1_BUSY 0x01
#define W25Q64_JEDEC_MFR_WINBOND 0xEF
struct w25q64jv_device { SPIDriver *spip; SPIConfig spicfg; };
int w25q64jv_init(struct w25q64jv_device *dev, SPIDriver *spip);
int w25q64jv_probe(struct w25q64jv_device *dev);
int w25q64jv_read(struct w25q64jv_device *dev, uint32_t addr, uint8_t *buf, uint32_t len);
int w25q64jv_write(struct w25q64jv_device *dev, uint32_t addr, const uint8_t *buf, uint32_t len);
int w25q64jv_erase_sector(struct w25q64jv_device *dev, uint32_t addr);
#endif
