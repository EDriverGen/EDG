#include "max31855_ref.h"
#include "hdf_log.h"
#define HDF_LOG_TAG max31855

int32_t max31855_init(struct max31855_device *dev, uint32_t bus, uint32_t cs)
{
    struct SpiDevInfo info = { .busNum = bus, .csNum = cs };
    if (!dev) return HDF_ERR_INVALID_PARAM;
    dev->spi_handle = SpiOpen(&info);
    if (!dev->spi_handle) return HDF_FAILURE;
    return HDF_SUCCESS;
}

void max31855_deinit(struct max31855_device *dev)
{
    if (dev && dev->spi_handle) { SpiClose(dev->spi_handle); dev->spi_handle = NULL; }
}

int32_t max31855_read_raw(struct max31855_device *dev, uint32_t *raw)
{
    uint8_t buf[4] = {0};
    struct SpiMsg msg = { .wbuf=NULL, .rbuf=buf, .len=4, .speed=5000000, .keepCs=0 };
    if (!dev || !dev->spi_handle || !raw) return HDF_ERR_INVALID_PARAM;
    if (SpiTransfer(dev->spi_handle, &msg, 1) != HDF_SUCCESS) return HDF_FAILURE;
    *raw = ((uint32_t)buf[0] << 24) |
           ((uint32_t)buf[1] << 16) |
           ((uint32_t)buf[2] << 8)  |
           ((uint32_t)buf[3]);
    return HDF_SUCCESS;
}

int max31855_has_fault(uint32_t raw) { return (raw & MAX31855_FAULT_BIT) ? 1 : 0; }
uint8_t max31855_get_fault(uint32_t raw) { return (uint8_t)(raw & 7); }

int32_t max31855_get_thermocouple_temp(uint32_t raw, int32_t *temp_mc)
{
    int32_t val;
    if (temp_mc == NULL) return HDF_ERR_INVALID_PARAM;
    if (raw & MAX31855_FAULT_BIT) return HDF_ERR_INVALID_PARAM;
    val = (int32_t)(raw >> 18);
    if (val & 0x2000) val |= ~((uint32_t)0x3FFF);
    *temp_mc = val * 250;
    return HDF_SUCCESS;
}

int32_t max31855_get_internal_temp(uint32_t raw, int32_t *temp_mc)
{
    int32_t val;
    if (temp_mc == NULL) return HDF_ERR_INVALID_PARAM;
    val = (int32_t)((raw >> 4) & 0x0FFF);
    if (val & 0x0800) val |= ~((uint32_t)0x0FFF);
    *temp_mc = (val * 625) / 10;
    return HDF_SUCCESS;
}

int32_t max31855_read_thermocouple(struct max31855_device *dev, int32_t *temp_mc)
{
    uint32_t raw; int32_t ret = max31855_read_raw(dev, &raw);
    if (ret != HDF_SUCCESS) return ret; return max31855_get_thermocouple_temp(raw, temp_mc);
}

int32_t max31855_read_internal(struct max31855_device *dev, int32_t *temp_mc)
{
    uint32_t raw; int32_t ret = max31855_read_raw(dev, &raw);
    if (ret != HDF_SUCCESS) return ret; return max31855_get_internal_temp(raw, temp_mc);
}
