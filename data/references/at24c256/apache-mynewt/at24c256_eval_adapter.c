#include "drivergen_eval_adapter.h"
#include "at24c256_ref.h"

static struct at24c256_device g_dev;

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "at24c256",
    .eval_class         = DRIVERGEN_EVAL_CLASS_MEMORY,
    .channel_count      = 0,
    .channels           = NULL,
    .primary_id         = NULL,
    .primary_unit       = NULL,
    .memory_size_bytes  = AT24C256_MEM_SIZE,
    .memory_page_bytes  = AT24C256_PAGE_SIZE,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

int drivergen_eval_init(const char *bus_name)
{
    (void)bus_name;
    return at24c256_init(&g_dev, 0, AT24C256_ADDR_DEFAULT) == 0
        ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_mem_read(uint32_t addr, uint8_t *buf, uint16_t len)
{
    if (buf == 0 || addr + (uint32_t)len > AT24C256_MEM_SIZE) return DRIVERGEN_EVAL_ERR_INVALID;
    return at24c256_read(&g_dev, (uint16_t)addr, buf, len) == 0
        ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_mem_write(uint32_t addr, const uint8_t *buf, uint16_t len)
{
    if (buf == 0 || addr + (uint32_t)len > AT24C256_MEM_SIZE) return DRIVERGEN_EVAL_ERR_INVALID;
    return at24c256_write(&g_dev, (uint16_t)addr, buf, len) == 0
        ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_cleanup(void)
{
    return DRIVERGEN_EVAL_OK;
}
