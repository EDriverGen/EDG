/*
 * W25Q64JV 64M-bit SPI NOR Flash reference driver for RT-Thread.
 */
#ifndef REFERENCE_W25Q64JV_RTTHREAD_H_
#define REFERENCE_W25Q64JV_RTTHREAD_H_

#include <rtthread.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C" {
#endif

#define W25Q64JV_SPI_MAX_HZ      80000000
#define W25Q64JV_PAGE_SIZE       256
#define W25Q64JV_SECTOR_SIZE     4096
#define W25Q64JV_SIZE            8388608

/* --- Command set --- */
#define W25Q64_CMD_WRITE_ENABLE     0x06
#define W25Q64_CMD_WRITE_DISABLE    0x04
#define W25Q64_CMD_READ_SR1         0x05
#define W25Q64_CMD_READ_DATA        0x03
#define W25Q64_CMD_FAST_READ        0x0B
#define W25Q64_CMD_PAGE_PROGRAM     0x02
#define W25Q64_CMD_SECTOR_ERASE     0x20
#define W25Q64_CMD_CHIP_ERASE       0xC7
#define W25Q64_CMD_JEDEC_ID         0x9F
#define W25Q64_CMD_POWER_DOWN       0xB9
#define W25Q64_CMD_RELEASE_POWERDOWN 0xAB

#define W25Q64_SR1_BUSY             0x01
#define W25Q64_JEDEC_MFR_WINBOND    0xEF
#define W25Q64_JEDEC_TYPE_W25Q64JV  0x40
#define W25Q64_JEDEC_CAP_W25Q64JV   0x17

struct w25q64jv_device {
    struct rt_spi_device *spi;
};

rt_err_t w25q64jv_init(struct w25q64jv_device *dev, const char *spi_device_name);
rt_err_t w25q64jv_probe(struct w25q64jv_device *dev);
rt_err_t w25q64jv_read(struct w25q64jv_device *dev, uint32_t addr,
                       uint8_t *buf, uint32_t len);
rt_err_t w25q64jv_write(struct w25q64jv_device *dev, uint32_t addr,
                        const uint8_t *buf, uint32_t len);
rt_err_t w25q64jv_erase_sector(struct w25q64jv_device *dev, uint32_t addr);

#ifdef __cplusplus
}
#endif
#endif
