#include "drivergen_eval_adapter.h"
#include "hcsr04_ref.h"

static struct hcsr04_device g_dev;

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "hcsr04",
    .eval_class         = DRIVERGEN_EVAL_CLASS_SINGLE_CHANNEL,
    .channel_count      = 0,
    .channels           = NULL,
    .primary_id         = "distance",
    .primary_unit       = "mm",
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

static int parse_pin_n(const char *s, int n)
{
    int pin = 0;
    if (s == 0 || n < 3 || s[0] != 'P' || s[1] < 'A' || s[1] > 'H') {
        return -1;
    }
    for (int i = 2; i < n; i++) {
        if (s[i] < '0' || s[i] > '9') return -1;
        pin = pin * 10 + (s[i] - '0');
    }
    return pin > 15 ? -1 : ((s[1] - 'A') * 16 + pin);
}

static int parse_bus(const char *s, int *trig, int *echo)
{
    int len = 0;
    int colon = -1;
    if (s == 0) return -1;
    while (s[len] != '\0') {
        if (s[len] == ':') colon = len;
        len++;
    }
    if (colon < 0) {
        *trig = parse_pin_n(s, len);
        *echo = *trig + 1;
        return *trig < 0 || (*echo % 16) == 0 ? -1 : 0;
    }
    *trig = parse_pin_n(s, colon);
    *echo = parse_pin_n(s + colon + 1, len - colon - 1);
    return *trig < 0 || *echo < 0 ? -1 : 0;
}

int drivergen_eval_init(const char *bus_name)
{
    int trig = 0;
    int echo = 0;
    if (parse_bus(bus_name, &trig, &echo) != 0) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    return hcsr04_init(&g_dev, trig, echo) == 0
        ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_raw_i32(int32_t *out)
{
    int32_t mm = 0;
    if (out == 0) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (hcsr04_read_distance_mm(&g_dev, &mm) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    *out = mm;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void)
{
    return DRIVERGEN_EVAL_OK;
}
