#ifndef __DPS310_REF_H
#define __DPS310_REF_H

#include "tos_k.h"
#ifndef HAL_I2C_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
#endif
#include "stm32f1xx_hal.h"
#include <stdint.h>

#define DPS310_ADDR_LOW           0x76
#define DPS310_ADDR_HIGH          0x77
#define DPS310_DEFAULT_ADDR       DPS310_ADDR_HIGH
#define DPS310_REG_PSR_B2         0x00
#define DPS310_REG_TMP_B2         0x03
#define DPS310_REG_PRS_CFG        0x06
#define DPS310_REG_TMP_CFG        0x07
#define DPS310_REG_MEAS_CFG       0x08
#define DPS310_REG_CFG_REG        0x09
#define DPS310_REG_RESET          0x0C
#define DPS310_REG_PROD_ID        0x0D
#define DPS310_REG_COEF_START     0x10
#define DPS310_REG_COEF_SRCE      0x28

#define DPS310_MEAS_COEF_RDY      (1U << 7)
#define DPS310_MEAS_SENSOR_RDY    (1U << 6)
#define DPS310_MEAS_TMP_RDY       (1U << 5)
#define DPS310_MEAS_PRS_RDY       (1U << 4)
#define DPS310_MEAS_PRS_SINGLE    0x01
#define DPS310_MEAS_TMP_SINGLE    0x02
#define DPS310_RESET_SOFT         0x89

#define DPS310_PROD_ID_MASK       0x0F
#define DPS310_PROD_ID_EXPECTED   0x00
#define DPS310_SCALE_FACTOR_1     524288.0f

struct dps310_device {
    I2C_HandleTypeDef *bus;
    uint16_t addr;
    int32_t c0, c1, c00, c10, c01, c11, c20, c21, c30;
    uint8_t coef_ready;
    uint8_t temp_cfg;
};

int dps310_init(struct dps310_device *dev, I2C_HandleTypeDef *bus, uint16_t addr);
int dps310_probe(struct dps310_device *dev);
int dps310_read_calibration(struct dps310_device *dev);
int dps310_read_pressure(struct dps310_device *dev, int32_t *pressure_pa);
int dps310_read_temperature(struct dps310_device *dev, int32_t *temp_mcelsius);

#endif
