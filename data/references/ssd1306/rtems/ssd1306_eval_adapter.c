#include "drivergen_eval_adapter.h"
#include "ssd1306_ref.h"

static struct ssd1306_device g_dev;

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "ssd1306",
    .eval_class         = DRIVERGEN_EVAL_CLASS_DISPLAY,
    .channel_count      = 0,
    .channels           = NULL,
    .primary_id         = NULL,
    .primary_unit       = NULL,
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

int drivergen_eval_init(const char *bus_name)
{
    const char *path = (bus_name != 0 && bus_name[0] != '\0') ? bus_name : "/dev/i2c-0";
    return ssd1306_init(&g_dev, path, SSD1306_I2C_ADDR) == 0
        ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_output_frame(const uint8_t *data, uint16_t len)
{
    if (data == 0 || len == 0) return DRIVERGEN_EVAL_ERR_INVALID;
    return ssd1306_write_frame(&g_dev, data, len) == 0
        ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_status(uint8_t *out)
{
    (void)out;
    return DRIVERGEN_EVAL_ERR_UNSUPPORTED;
}

int drivergen_eval_cleanup(void)
{
    ssd1306_deinit(&g_dev);
    return DRIVERGEN_EVAL_OK;
}
