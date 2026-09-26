#ifndef W25Q64JV_H
#define W25Q64JV_H

#include <stdint.h>
#include <stddef.h>

#define W25Q64JV_CMD_READ_DATA        0x03u
#define W25Q64JV_CMD_PAGE_PROGRAM     0x02u
#define W25Q64JV_CMD_SECTOR_ERASE_4K  0x20u
#define W25Q64JV_CMD_WRITE_ENABLE     0x06u
#define W25Q64JV_CMD_READ_STATUS_1    0x05u
#define W25Q64JV_CMD_JEDEC_ID         0x9Fu

#define W25Q64JV_PAGE_SIZE            256u
#define W25Q64JV_MEMORY_SIZE          8388608u

#define W25Q64JV_STATUS_BUSY          0x01u
#define W25Q64JV_STATUS_WEL           0x02u

#define W25Q64JV_OK                   0
#define W25Q64JV_ERR_PARAM           (-22)
#define W25Q64JV_ERR_IO              (-5)
#define W25Q64JV_ERR_TIMEOUT         (-110)
#define W25Q64JV_ERR_PERM            (-1)
#define W25Q64JV_ERR_ACCES           (-13)

#define W25Q64JV_SPI_TIMEOUT_MS       1000u
#define W25Q64JV_PROGRAM_TIMEOUT_MS   5000u
#define W25Q64JV_ERASE_TIMEOUT_MS     1000u

#define W25Q64JV_JEDEC_MFR            0xEFu
#define W25Q64JV_JEDEC_DEVICE         0x40u
#define W25Q64JV_JEDEC_CAPACITY       0x17u

typedef struct {
    void *bus_handle;
    uint8_t jedec_mfr;
    uint8_t jedec_device;
    uint8_t jedec_capacity;
    uint8_t initialized;
} w25q64jv_dev_t;

int w25q64jv_init(w25q64jv_dev_t *dev, void *bus_handle);
int w25q64jv_read(w25q64jv_dev_t *dev, uint32_t addr, uint8_t *buf, uint32_t len);
int w25q64jv_write(w25q64jv_dev_t *dev, uint32_t addr, const uint8_t *buf, uint32_t len);
int w25q64jv_read_jedec_id(w25q64jv_dev_t *dev, uint8_t *mfr, uint8_t *device, uint8_t *capacity);
int w25q64jv_read_status(w25q64jv_dev_t *dev, uint8_t *status);
int w25q64jv_write_enable(w25q64jv_dev_t *dev);
int w25q64jv_wait_ready(w25q64jv_dev_t *dev, uint32_t timeout_ms);
int w25q64jv_erase_sector(w25q64jv_dev_t *dev, uint32_t addr);

#endif /* W25Q64JV_H */