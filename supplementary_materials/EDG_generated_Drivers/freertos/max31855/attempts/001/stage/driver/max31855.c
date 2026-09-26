#include "max31855.h"
#include <stdint.h>
#include <string.h>

#include "freertos.h"
#include "stm32f1xx_hal.h"
int max31855_init(struct max31855_dev *dev, void *bus_handle)
{
    dev->bus_handle = (SPI_HandleTypeDef *)bus_handle;
    HAL_Delay(200);
    return 0;
}

static int32_t convert_thermocouple(uint16_t raw_14)
{
    int32_t sign = (raw_14 >> 13) & 1;
    int32_t magnitude = raw_14 & 0x1FFF;
    if (sign) {
        magnitude = magnitude ^ 0x2000;
    }
    int32_t value = sign * (-8192) + magnitude;
    return (value * 250) / 1000;
}

static int32_t convert_internal(uint16_t raw_12)
{
    int32_t sign = (raw_12 >> 11) & 1;
    int32_t magnitude = raw_12 & 0x7FF;
    if (sign) {
        magnitude = magnitude ^ 0x800;
    }
    int32_t value = sign * (-2048) + magnitude;
    return (value * 625) / 10000;
}

int max31855_read_temperatures(struct max31855_dev *dev, int32_t *tc, int32_t *ti)
{
    uint8_t tx_buf[4] = {0};
    uint8_t rx_buf[4];
    HAL_StatusTypeDef ret;

    ret = HAL_SPI_TransmitReceive(dev->bus_handle, tx_buf, rx_buf, 4, 100);
    if (ret != HAL_OK) {
        return -1;
    }

    uint32_t frame = ((uint32_t)rx_buf[0] << 24) |
                     ((uint32_t)rx_buf[1] << 16) |
                     ((uint32_t)rx_buf[2] << 8) |
                     (uint32_t)rx_buf[3];

    if (frame & 0x00010000) {
        return -1;
    }

    uint16_t raw_14 = (frame >> 18) & 0x3FFF;
    uint16_t raw_12 = (frame >> 4) & 0xFFF;

    *tc = convert_thermocouple(raw_14);
    *ti = convert_internal(raw_12);

    return 0;
}
