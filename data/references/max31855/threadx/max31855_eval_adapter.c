/* max31855_eval_adapter.c — Evaluation adapter for threadx */
#include "drivergen_eval_adapter.h"
#include "max31855_ref.h"
#include "threadx.h"

static SPI_HandleTypeDef _hspi;

static int _tx_spi_recv(void *ctx, uint8_t *buf, uint32_t len) {
    return HAL_SPI_Receive((SPI_HandleTypeDef *)ctx, buf, (uint16_t)len, 100) == HAL_OK ? 0 : -1;
}
static void _tx_cs_select(void *ctx) {
    (void)ctx; HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
}
static void _tx_cs_deselect(void *ctx) {
    (void)ctx; HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
}
static const struct max31855_spi_ops _spi_ops = {
    .spi_recv = _tx_spi_recv,
    .cs_select = _tx_cs_select,
    .cs_deselect = _tx_cs_deselect,
};

static struct max31855_device g_eval_dev;

const drivergen_eval_meta_t drivergen_eval_meta = {
    .device_id          = "max31855",
    .eval_class         = DRIVERGEN_EVAL_CLASS_SINGLE_CHANNEL,
    .channel_count      = 0,
    .channels           = NULL,
    .primary_id         = "temp_thermocouple",
    .primary_unit       = "mC",
    .memory_size_bytes  = 0,
    .memory_page_bytes  = 0,
    .abi_version_major  = DRIVERGEN_EVAL_ABI_VERSION_MAJOR,
    .abi_version_minor  = DRIVERGEN_EVAL_ABI_VERSION_MINOR,
};

int drivergen_eval_init(const char *bus_name) {
    (void)bus_name;
    HAL_SPI_Init(&_hspi);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
    int err = max31855_init(&g_eval_dev, &_spi_ops, &_hspi);
    return (err == 0) ? DRIVERGEN_EVAL_OK : DRIVERGEN_EVAL_ERR_IO;
}

int drivergen_eval_read_raw_i32(int32_t *out) {
    if (out == NULL) {
        return DRIVERGEN_EVAL_ERR_INVALID;
    }
    int32_t mc = 0;
    int err = max31855_read_thermocouple(&g_eval_dev, &mc);
    if (err != 0) {
        return DRIVERGEN_EVAL_ERR_IO;
    }
    *out = (int32_t)mc;
    return DRIVERGEN_EVAL_OK;
}

int drivergen_eval_cleanup(void) {
    return DRIVERGEN_EVAL_OK;
}
