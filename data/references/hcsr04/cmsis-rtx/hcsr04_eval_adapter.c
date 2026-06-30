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

static int parse_pin_n(const char *s, int n, GPIO_TypeDef **port, uint16_t *pin)
{
    int p = 0;
    int idx = 0;
    if (s == 0 || n < 3 || s[0] != 'P' || port == 0 || pin == 0) {
        return -1;
    }
    if (s[1] < 'A' || s[1] > 'D') {
        return -1;
    }
    p = s[1] - 'A';
    for (int i = 2; i < n; i++) {
        if (s[i] < '0' || s[i] > '9') {
            return -1;
        }
        idx = idx * 10 + (s[i] - '0');
    }
    if (idx < 0 || idx > 15) {
        return -1;
    }
    *port = p == 0 ? GPIOA : (p == 1 ? GPIOB : (p == 2 ? GPIOC : GPIOD));
    *pin = (uint16_t)(1U << idx);
    return 0;
}

static int parse_bus(const char *s, GPIO_TypeDef **trig_port, uint16_t *trig_pin,
                     GPIO_TypeDef **echo_port, uint16_t *echo_pin)
{
    int len = 0;
    int colon = -1;
    if (s == 0) {
        return -1;
    }
    while (s[len] != '\0') {
        if (s[len] == ':') colon = len;
        len++;
    }
    if (colon < 0) {
        if (parse_pin_n(s, len, trig_port, trig_pin) != 0) return -1;
        *echo_port = *trig_port;
        *echo_pin = (uint16_t)(*trig_pin << 1);
        return *echo_pin == 0 ? -1 : 0;
    }
    return parse_pin_n(s, colon, trig_port, trig_pin) == 0 &&
           parse_pin_n(s + colon + 1, len - colon - 1, echo_port, echo_pin) == 0 ? 0 : -1;
}

int drivergen_eval_init(const char *bus_name)
{
    GPIO_TypeDef *tp = 0;
    GPIO_TypeDef *ep = 0;
    uint16_t tpin = 0;
    uint16_t epin = 0;
    if (parse_bus(bus_name, &tp, &tpin, &ep, &epin) != 0) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    return hcsr04_init(&g_dev, tp, tpin, ep, epin) == 0
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
