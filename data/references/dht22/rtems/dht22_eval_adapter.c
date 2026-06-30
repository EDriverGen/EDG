#include "drivergen_eval_adapter.h"
#include "dht22_ref.h"

#define DHT22_EVAL_CHANNEL_COUNT 2

static struct dht22_device g_dev;
static int32_t g_humidity_x10;
static int32_t g_temp_x10;
static int g_sample_valid;

static const drivergen_eval_channel_t g_channels[DHT22_EVAL_CHANNEL_COUNT] = {
    {"humidity", "pctRH_x10", 0},
    {"temp", "mC_x10", 0},
};

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "dht22",
    .eval_class         = DRIVERGEN_EVAL_CLASS_MULTI_CHANNEL,
    .channel_count      = DHT22_EVAL_CHANNEL_COUNT,
    .channels           = g_channels,
    .primary_id         = "humidity",
    .primary_unit       = "pctRH_x10",
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

static int refresh(void)
{
    int16_t t = 0;
    uint16_t h = 0;
    if (dht22_read(&g_dev, &t, &h) != 0) return DRIVERGEN_EVAL_ERR_IO;
    g_temp_x10 = t;
    g_humidity_x10 = h;
    g_sample_valid = 1;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_init(const char *bus_name)
{
    int pin = parse_pin(bus_name);
    g_sample_valid = 0;
    if (pin < 0) return DRIVERGEN_EVAL_ERR_INVALID;
    return dht22_init(&g_dev, pin) == 0 ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_channel(int channel_id, int32_t *out)
{
    if (out == 0) return DRIVERGEN_EVAL_ERR_INVALID;
    if (channel_id < 0 || channel_id >= DHT22_EVAL_CHANNEL_COUNT) return DRIVERGEN_EVAL_ERR_INVALID;
    if (channel_id == 0 || !g_sample_valid) {
        int err = refresh();
        if (err != DRIVERGEN_EVAL_OK) return err;
    }
    *out = channel_id == 0 ? g_humidity_x10 : g_temp_x10;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void)
{
    g_sample_valid = 0;
    return DRIVERGEN_EVAL_OK;
}
