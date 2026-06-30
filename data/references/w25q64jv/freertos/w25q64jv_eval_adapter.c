/* W25Q64JV eval adapter for freertos — test_main_memory harness. */
#include "drivergen_eval_adapter.h"
#include "w25q64jv_ref.h"
static struct w25q64jv_device g_eval_dev;
int drivergen_eval_init(const char *bus_name) {
    (void)bus_name;
    static SPI_HandleTypeDef _hspi;
    HAL_SPI_Init(&_hspi);
    return (w25q64jv_init(&g_eval_dev, &_hspi, GPIOA, GPIO_PIN_4) == 0)
           ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}
int drivergen_eval_probe(void) { return (w25q64jv_probe(&g_eval_dev)==0)?DRIVERGEN_EVAL_OK:DRIVERGEN_EVAL_ERR_IO; }
int drivergen_eval_mem_read(uint32_t a, uint8_t *b, uint16_t l) {
    if(!b||!l)return DRIVERGEN_EVAL_ERR_INVALID;
    return (w25q64jv_read(&g_eval_dev,a,b,l)==0)?DRIVERGEN_EVAL_OK:DRIVERGEN_EVAL_ERR_IO;
}
int drivergen_eval_mem_write(uint32_t a, const uint8_t *b, uint16_t l) {
    if(!b||!l)return DRIVERGEN_EVAL_ERR_INVALID;
    w25q64jv_erase_sector(&g_eval_dev,a);
    return (w25q64jv_write(&g_eval_dev,a,b,l)==0)?DRIVERGEN_EVAL_OK:DRIVERGEN_EVAL_ERR_IO;
}
int drivergen_eval_cleanup(void) { return DRIVERGEN_EVAL_OK; }
const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id="w25q64jv",.eval_class=DRIVERGEN_EVAL_CLASS_MEMORY,.channel_count=0,.channels=NULL,
    .primary_id="",.primary_unit="",.memory_size_bytes=W25Q64JV_SIZE,.memory_page_bytes=W25Q64JV_PAGE_SIZE,
    .abi_version_major=DRIVERGEN_EVAL_ABI_VERSION_MAJOR,.abi_version_minor=DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};
