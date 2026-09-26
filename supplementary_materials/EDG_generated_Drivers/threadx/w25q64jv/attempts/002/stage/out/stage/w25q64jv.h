#ifndef W25Q64JV_H
#define W25Q64JV_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    void *bus_handle;
    uint8_t jedec_mfr;
    uint8_t jedec_mem_type;
    uint8_t jedec_capacity;
    uint8_t initialized;
} w25q64jv_dev_t;

int w25q64jv_init(w25q64jv_dev_t *dev, void *bus_handle);
int w25q64jv_read(w25q64jv_dev_t *dev, uint32_t addr, uint8_t *buf, uint32_t len);
int w25q64jv_write(w25q64jv_dev_t *dev, uint32_t addr, const uint8_t *buf, uint32_t len);
int w25q64jv_read_jedec_id(w25q64jv_dev_t *dev, uint8_t *mfr, uint8_t *mem_type, uint8_t *capacity);
int w25q64jv_read_status1(w25q64jv_dev_t *dev, uint8_t *status);
int w25q64jv_wait_ready(w25q64jv_dev_t *dev, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif /* W25Q64JV_H */