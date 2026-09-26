/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * TMP421 Remote Temperature Sensor Driver for XiUOS
 */
#ifndef __TMP421_REF_H
#define __TMP421_REF_H

#include <transform.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define TMP421_ADDR_DEFAULT          0x2A

#define TMP421_REG_LOCAL_TEMP_HI     0x00
#define TMP421_REG_LOCAL_TEMP_LO     0x10
#define TMP421_REG_REMOTE_TEMP_HI    0x01
#define TMP421_REG_REMOTE_TEMP_LO    0x11
#define TMP421_REG_CONFIG_1          0x09
#define TMP421_REG_MANUFACTURER_ID   0xFE
#define TMP421_REG_DEVICE_ID         0xFF

#define TMP421_MANUFACTURER_ID_TI    0x55
#define TMP421_CONFIG1_RANGE         (1U << 2)

struct tmp421_device
{
  int fd;
  uint16_t addr;
};

int tmp421_init(struct tmp421_device *dev,
                const char *i2c_dev_path,
                uint16_t addr);
void tmp421_deinit(struct tmp421_device *dev);
int tmp421_probe(struct tmp421_device *dev);
int tmp421_read_local_temp(struct tmp421_device *dev,
                           int32_t *temp_mcelsius);
int tmp421_read_remote_temp(struct tmp421_device *dev,
                            int32_t *temp_mcelsius);

#ifdef __cplusplus
}
#endif

#endif /* __TMP421_REF_H */
