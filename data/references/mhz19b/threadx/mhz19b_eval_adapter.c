/* mhz19b_eval_adapter.c — Evaluation adapter for threadx */
#include "drivergen_eval_adapter.h"
#include "mhz19b_ref.h"
#include "threadx.h"

static UART_HandleTypeDef _huart;

static int _tx_uart_send(void *ctx, const uint8_t *data, uint16_t len) {
    return HAL_UART_Transmit((UART_HandleTypeDef *)ctx, (uint8_t *)data, len, 100) == HAL_OK ? 0 : -1;
}
static int _tx_uart_recv(void *ctx, uint8_t *data, uint16_t len, uint32_t timeout_ms) {
    return HAL_UART_Receive((UART_HandleTypeDef *)ctx, data, len, timeout_ms) == HAL_OK ? 0 : -1;
}
static const struct mhz19b_uart_ops _uart_ops = {
    .uart_send = _tx_uart_send,
    .uart_recv = _tx_uart_recv,
};

static struct mhz19b_device g_eval_dev;

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "mhz19b",
    .eval_class         = DRIVERGEN_EVAL_CLASS_SINGLE_CHANNEL,
    .channel_count      = 0,
    .channels           = NULL,
    .primary_id         = "co2",
    .primary_unit       = "ppm",
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

int drivergen_eval_init(const char *bus_name) {
    (void)bus_name;
    int err = mhz19b_init(&g_eval_dev, &_uart_ops, &_huart);
    return (err == 0) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_raw_i32(int32_t *out) {
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    uint16_t ppm = 0;
    int err = mhz19b_read_co2(&g_eval_dev, &ppm);
    if (err != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    *out = (int32_t)ppm;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void) {
    return DRIVERGEN_EVAL_OK;
}
