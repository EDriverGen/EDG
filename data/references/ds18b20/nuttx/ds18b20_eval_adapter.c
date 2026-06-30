/* ds18b20_eval_adapter.c — Evaluation adapter for nuttx */
#include "drivergen_eval_adapter.h"
#include "ds18b20_ref.h"
#include "nuttx.h"

static struct ds18b20_device g_eval_dev;

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

/* Parse "PA0".."PH15" into RT-Thread GET_PIN(port,pin) = port*16+pin.
 * Returns -1 on parse failure. Matches dht22 / hcsr04 convention. */
static int ds18b20_eval_parse_pin(const char *s) {
    if (s == NULL || s[0] != 'P') return -1;
    char port_c = s[1];
    if (port_c < 'A' || port_c > 'H') return -1;
    int port = (int)(port_c - 'A');
    int pin = 0;
    for (int i = 2; s[i] != '\0'; i++) {
        if (s[i] < '0' || s[i] > '9') return -1;
        pin = pin * 10 + (s[i] - '0');
        if (pin > 15) return -1;
    }
    return port * 16 + pin;
}

int drivergen_eval_init(const char *bus_name) {
    int err = ds18b20_init(&g_eval_dev, bus_name);
    return (err == 0) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_raw_i32(int32_t *out) {
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    int32_t cc = 0;  /* centicelsius */
    if (ds18b20_read_temp(&g_eval_dev, &cc) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    *out = cc * 10;  /* cC → mC */
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void) {
    return DRIVERGEN_EVAL_OK;
}
