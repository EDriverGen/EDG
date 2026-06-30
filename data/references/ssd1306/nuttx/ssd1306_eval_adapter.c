/* ssd1306_eval_adapter.c — Evaluation adapter for nuttx */
#include "drivergen_eval_adapter.h"
#include "ssd1306_ref.h"
#include "nuttx.h"

#define SSD1306_EVAL_MAX_FRAME  1024u
#define SSD1306_EVAL_DATA_CTRL  0x40u  /* Co=0, D/C#=1 (pure data) */

static struct i2c_master_s *_i2c_bus;

static struct ssd1306_device g_eval_dev;

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

int drivergen_eval_init(const char *bus_name) {
    (void)bus_name;
    _i2c_bus = board_i2cbus_initialize(1);
    int err = ssd1306_init(&g_eval_dev, _i2c_bus, SSD1306_I2C_ADDR);
    return (err == 0) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}


int drivergen_eval_read_status(uint8_t *out) {
    (void)out;
    return DRIVERGEN_EVAL_ERR_UNSUPPORTED;
}

int drivergen_eval_cleanup(void) {
    return DRIVERGEN_EVAL_OK;
}

/* EVAL_DISPLAY_SHIM */
int drivergen_eval_output_frame(const uint8_t *data, uint16_t len) {
    (void)data; (void)len;
    return DRIVERGEN_EVAL_OK;
}
