#include "drivergen_eval_adapter.h"
#include "ds18b20_ref.h"

static struct ds18b20_device g_dev;

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "ds18b20",
    .eval_class         = DRIVERGEN_EVAL_CLASS_SINGLE_CHANNEL,
    .channel_count      = 0,
    .channels           = NULL,
    .primary_id         = "temp",
    .primary_unit       = "mC",
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

static int parse_pin(const char *s)
{
    int pin = 0;
    if (s == 0 || s[0] != 'P' || s[1] < 'A' || s[1] > 'H') return -1;
    for (int i = 2; s[i] != '\0'; i++) {
        if (s[i] < '0' || s[i] > '9') return -1;
        pin = pin * 10 + (s[i] - '0');
    }
    return pin > 15 ? -1 : ((s[1] - 'A') * 16 + pin);
}

int drivergen_eval_init(const char *bus_name)
{
    int pin = parse_pin(bus_name);
    if (pin < 0) return DRIVERGEN_EVAL_ERR_INVALID;
    return ds18b20_init(&g_dev, pin) == 0 ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_raw_i32(int32_t *out)
{
    int32_t cc = 0;
    if (out == 0) return DRIVERGEN_EVAL_ERR_INVALID;
    if (ds18b20_read_temp(&g_dev, &cc) != 0) return DRIVERGEN_EVAL_ERR_IO;
    *out = cc * 10;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void)
{
    return DRIVERGEN_EVAL_OK;
}
