/* hcsr04_eval_adapter.c — Evaluation adapter for zephyr */
#include "drivergen_eval_adapter.h"
#include "hcsr04_ref.h"
#include "zephyr.h"

static struct gpio_dt_spec _trig_gpio = {.port = NULL, .pin = 5, .dt_flags = 0};
static struct gpio_dt_spec _echo_gpio = {.port = NULL, .pin = 6, .dt_flags = 0};

static struct hcsr04_device g_eval_dev;

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

/* Parse "PA0".."PH15" into port*16+pin; returns -1 on failure. */
static int hcsr04_eval_parse_pin_n(const char *s, int n) {
    if (s == NULL || n < 3 || s[0] != 'P') return -1;
    char port_c = s[1];
    if (port_c < 'A' || port_c > 'H') return -1;
    int port = (int)(port_c - 'A');
    int pin = 0;
    for (int i = 2; i < n; i++) {
        if (s[i] < '0' || s[i] > '9') return -1;
        pin = pin * 10 + (s[i] - '0');
        if (pin > 15) return -1;
    }
    return port * 16 + pin;
}

static int hcsr04_eval_parse_pin(const char *s) {
    if (s == NULL) return -1;
    int n = 0; while (s[n] != '\0') n++;
    return hcsr04_eval_parse_pin_n(s, n);
}

/* Parse "PB5:PB6" → trig=21, echo=22. Returns 0 on success, -1 on fail. */
static int hcsr04_eval_parse_trig_echo(const char *s,
                                       int *out_trig, int *out_echo) {
    if (s == NULL) return -1;
    int colon = -1;
    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] == ':') { colon = i; break; }
    }
    if (colon < 0) {
        /* Single pin: echo = trig + 1. */
        int trig = hcsr04_eval_parse_pin(s);
        if (trig < 0) return -1;
        *out_trig = trig;
        *out_echo = trig + 1;
        return 0;
    }
    int trig = hcsr04_eval_parse_pin_n(s, colon);
    int echo = hcsr04_eval_parse_pin(s + colon + 1);
    if (trig < 0 || echo < 0) return -1;
    *out_trig = trig;
    *out_echo = echo;
    return 0;
}

int drivergen_eval_init(const char *bus_name) {
    (void)bus_name;
    int err = hcsr04_init(&g_eval_dev, &_trig_gpio, &_echo_gpio);
    return (err == 0) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_raw_i32(int32_t *out) {
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    int32_t mm = 0;
    if (hcsr04_read_distance_mm(&g_eval_dev, &mm) != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    *out = mm;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void) {
    return DRIVERGEN_EVAL_OK;
}
