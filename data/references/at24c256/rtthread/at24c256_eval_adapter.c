/* at24c256_eval_adapter.c
 *
 * Hand-written DriverGen Evaluation ABI adapter for the AT24C256
 * RT-Thread reference driver. Provides the minimal eval ABI surface for
 * the "memory" eval_class.
 *
 * Reference driver API:
 *   int at24c256_init (dev, struct rt_i2c_bus_device *bus, uint16_t addr);
 *   int at24c256_read (dev, uint16_t mem_addr, uint8_t *data, uint16_t len);
 *   int at24c256_write(dev, uint16_t mem_addr, const uint8_t *data, uint16_t len);
 *
 * Device facts (from AT24C256 datasheet):
 *   - 32 768 bytes total (256 Kbit)
 *   - 64-byte page boundary for writes
 *   - Two-byte memory address (MSB first)
 *   - 5 ms internal write cycle (reference driver already rt_thread_mdelay
 *     after each page, so the adapter does NOT need to replicate the wait)
 *
 * The adapter truncates (addr, len) requests that would run past the
 * last byte of the device.
 */
#include "drivergen_eval_adapter.h"
#include "at24c256_ref.h"

#define AT24C256_EVAL_MEM_SIZE   32768u
#define AT24C256_EVAL_PAGE_SIZE  64u

static struct at24c256_device g_eval_dev;

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "at24c256",
    .eval_class         = DRIVERGEN_EVAL_CLASS_MEMORY,
    .channel_count      = 0,
    .channels           = NULL,
    .primary_id         = NULL,
    .primary_unit       = NULL,
    .memory_size_bytes  = AT24C256_EVAL_MEM_SIZE,
    .memory_page_bytes  = AT24C256_EVAL_PAGE_SIZE,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

int drivergen_eval_init(const char *bus_name) {
    if (bus_name == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    rt_device_t bus_dev = rt_device_find(bus_name);
    if (bus_dev == RT_NULL) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    struct rt_i2c_bus_device *bus = (struct rt_i2c_bus_device *)bus_dev;
    if (at24c256_init(&g_eval_dev, bus, AT24C256_ADDR_DEFAULT) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_mem_read(uint32_t addr, uint8_t *buf, uint16_t len) {
    if (buf == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (addr >= AT24C256_EVAL_MEM_SIZE) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    uint32_t end = addr + (uint32_t)len;
    if (end > AT24C256_EVAL_MEM_SIZE) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (at24c256_read(&g_eval_dev, (uint16_t)addr, buf, len) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_mem_write(uint32_t addr, const uint8_t *buf, uint16_t len) {
    if (buf == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (addr >= AT24C256_EVAL_MEM_SIZE) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    uint32_t end = addr + (uint32_t)len;
    if (end > AT24C256_EVAL_MEM_SIZE) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (at24c256_write(&g_eval_dev, (uint16_t)addr, buf, len) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void) {
    return DRIVERGEN_EVAL_OK;
}
