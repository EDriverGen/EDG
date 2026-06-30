#ifndef __TMP421_REF_H
#define __TMP421_REF_H

#include "tos_k.h"
#ifndef HAL_I2C_MODULE_ENABLED
#define HAL_I2C_MODULE_ENABLED
#endif
#include "stm32f1xx_hal.h"
#include <stdint.h>

#define TMP421_ADDR_DEFAULT      0x2A
#define TMP421_REG_LOCAL_TEMP_H  0x00
#define TMP421_REG_LOCAL_TEMP_L  0x10
#define TMP421_REG_REMOTE_TEMP_H 0x01
#define TMP421_REG_REMOTE_TEMP_L 0x11
#define TMP421_REG_MFG_ID        0xFE
#define TMP421_REG_DEV_ID        0xFF
#define TMP421_MFG_ID_EXPECTED   0x55
#define TMP421_DEV_ID_EXPECTED   0x21

struct tmp421_device {
    I2C_HandleTypeDef *bus;
    uint16_t addr;
};

int tmp421_init(struct tmp421_device *dev, I2C_HandleTypeDef *bus, uint16_t addr);
int tmp421_probe(struct tmp421_device *dev);
int tmp421_read_local_temp(struct tmp421_device *dev, int32_t *temp_mcelsius);
int tmp421_read_remote_temp(struct tmp421_device *dev, int32_t *temp_mcelsius);

#endif
