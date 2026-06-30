/* ds18b20_eval_adapter.c
 *
 * Hand-written DriverGen Evaluation ABI adapter for the DS18B20 RT-Thread
 * reference driver (GPIO 1-Wire, single_channel temperature).
 *
 * Reference driver API:
 *   rt_err_t ds18b20_init(struct ds18b20_device *dev, rt_base_t data_pin);
 *   rt_err_t ds18b20_read_temp(struct ds18b20_device *dev, int32_t *temp_x100);
 *
 * `bus_name` is a pin label like "PB5"; we parse via the same
 * `port*16 + pin` convention as the DHT22 adapter.
 *
 * Unit mapping: the driver returns temp_x100 (centicelsius, 0.01 degC).
 * The oracle declares "mC", so the adapter multiplies by 10 to match.
 *
 * Runtime caveat (mirrors dht22 note): the `rt_pin_read` path currently
 * depends on `stubs_gpio.c` which, under the gpio pulse injector
 * platform, wires GPIOB_IDR into the Python slave. For ports that the
 * slave does not drive, reads return zero — which is acceptable for
 * the E17 compile-link baseline but not for end-to-end Renode runs.
 */
#include "drivergen_eval_adapter.h"
#include "ds18b20_ref.h"

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
    int pin_id = ds18b20_eval_parse_pin(bus_name);
    if (pin_id < 0) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    if (ds18b20_init(&g_eval_dev, (rt_base_t)pin_id) != RT_EOK) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_read_raw_i32(int32_t *out) {
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    int32_t cc = 0;  /* centicelsius */
    if (ds18b20_read_temp(&g_eval_dev, &cc) != RT_EOK) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    *out = cc * 10;  /* cC → mC */
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void) {
    return DRIVERGEN_EVAL_OK;
}
