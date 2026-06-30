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

static int parse_pin(const char *s, GPIO_TypeDef **port, uint16_t *pin)
{
    int idx = 0;
    int p;
    if (s == 0 || s[0] != 'P' || s[1] < 'A' || s[1] > 'D') return -1;
    p = s[1] - 'A';
    for (int i = 2; s[i] != '\0'; i++) {
        if (s[i] < '0' || s[i] > '9') return -1;
        idx = idx * 10 + (s[i] - '0');
    }
    if (idx > 15) return -1;
    *port = p == 0 ? GPIOA : (p == 1 ? GPIOB : (p == 2 ? GPIOC : GPIOD));
    *pin = (uint16_t)(1U << idx);
    return 0;
}

int drivergen_eval_init(const char *bus_name)
{
    GPIO_TypeDef *port = 0;
    uint16_t pin = 0;
    if (parse_pin(bus_name, &port, &pin) != 0) return DRIVERGEN_EVAL_ERR_INVALID;
    return ds18b20_init(&g_dev, port, pin) == 0 ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
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
