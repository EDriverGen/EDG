#include "drivergen_eval_adapter.h"
#include "mhz19b_ref.h"

static UART_HandleTypeDef g_uart;
static struct mhz19b_device g_dev;

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "mhz19b",
    .eval_class         = DRIVERGEN_EVAL_CLASS_SINGLE_CHANNEL,
    .channel_count      = 0,
    .channels           = NULL,
    .primary_id         = "co2",
    .primary_unit       = "ppm",
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

int drivergen_eval_init(const char *bus_name)
{
    (void)bus_name;
    return mhz19b_init(&g_dev, &g_uart) == 0
        ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_raw_i32(int32_t *out)
{
    uint16_t ppm = 0;
    if (out == 0) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (mhz19b_read_co2(&g_dev, &ppm) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    *out = (int32_t)ppm;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void)
{
    return DRIVERGEN_EVAL_OK;
}
