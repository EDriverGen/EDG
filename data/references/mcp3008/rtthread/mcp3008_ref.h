/*
 * MCP3008 10-bit SPI ADC driver for RT-Thread
 */
#ifndef DRIVERS_INCLUDE_MCP3008_H_
#define DRIVERS_INCLUDE_MCP3008_H_

#include <rtthread.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MCP3008_SPI_MAX_HZ   3600000
#define MCP3008_CHANNELS     8
#define MCP3008_MAX_VALUE    1023
#define MCP3008_SINGLE       1
#define MCP3008_DIFF         0

struct mcp3008_device
{
    struct rt_spi_device *spi;
    const char *device_name;
    rt_uint16_t vref_mv;
};

rt_err_t mcp3008_init(struct mcp3008_device *dev, const char *device_name, rt_uint16_t vref_mv);
rt_err_t mcp3008_read_raw(struct mcp3008_device *dev, rt_uint8_t channel, rt_uint8_t single, rt_uint16_t *raw);
rt_err_t mcp3008_read_voltage(struct mcp3008_device *dev, rt_uint8_t channel, rt_uint16_t *mv);
int mcp3008_to_millivolts(uint16_t raw, uint16_t vref_mv, uint16_t *mv);

#ifdef __cplusplus
}
#endif
#endif
