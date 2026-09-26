/*
 * SPDX-License-Identifier: MIT
 *
 * DPS310 Pressure/Temperature Sensor Driver for ThreadX
 */
#ifndef __DPS310_REF_H
#define __DPS310_REF_H

#include <tx_api.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DPS310_ADDR_LOW               0x76
#define DPS310_ADDR_HIGH              0x77
#define DPS310_DEFAULT_ADDR           0x77

#define DPS310_REG_PSR_B2             0x00
#define DPS310_REG_TMP_B2             0x03
#define DPS310_REG_PRS_CFG            0x06
#define DPS310_REG_TMP_CFG            0x07
#define DPS310_REG_MEAS_CFG           0x08
#define DPS310_REG_CFG_REG            0x09
#define DPS310_REG_RESET              0x0C
#define DPS310_REG_PROD_ID            0x0D
#define DPS310_REG_COEF_START         0x10
#define DPS310_REG_COEF_SRCE          0x28

#define DPS310_MEAS_COEF_RDY          (1U << 7)
#define DPS310_MEAS_SENSOR_RDY        (1U << 6)
#define DPS310_MEAS_TMP_RDY           (1U << 5)
#define DPS310_MEAS_PRS_RDY           (1U << 4)
#define DPS310_MEAS_PRS_SINGLE        0x01
#define DPS310_MEAS_TMP_SINGLE        0x02
#define DPS310_RESET_SOFT             0x89

#define DPS310_PROD_ID_MASK           0x0F
#define DPS310_PROD_ID_EXPECTED       0x00
#define DPS310_SCALE_FACTOR_1         524288.0f


struct dps310_i2c_ops
{
    int (*write)(void *context, uint16_t addr, const uint8_t *data, uint16_t len);
    int (*read)(void *context, uint16_t addr, uint8_t *data, uint16_t len);
    int (*write_read)(void *context, uint16_t addr,
                      const uint8_t *wdata, uint16_t wlen,
                      uint8_t *rdata, uint16_t rlen);
};

struct dps310_device
{
    void *bus_context;
    const struct dps310_i2c_ops *ops;
    uint16_t addr;
    int32_t c0, c1, c00, c10, c01, c11, c20, c21, c30;
    uint8_t coef_ready;
    uint8_t temp_cfg;
};

int dps310_init(struct dps310_device *dev, void *bus_context, const struct dps310_i2c_ops *ops, uint16_t addr);
int dps310_probe(struct dps310_device *dev);
int dps310_read_calibration(struct dps310_device *dev);
int dps310_read_pressure(struct dps310_device *dev, int32_t *pressure_pa);
int dps310_read_temperature(struct dps310_device *dev, int32_t *temp_mcelsius);

#ifdef __cplusplus
}
#endif

#endif /* __DPS310_REF_H */
