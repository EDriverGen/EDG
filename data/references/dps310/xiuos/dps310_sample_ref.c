/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * DPS310 Digital Pressure Sensor Sample for XiUOS
 */
#include <stdio.h>
#include <transform.h>
#include "dps310_ref.h"

#define I2C_DEV_PATH  "/dev/i2c1"

int dps310_sample(void)
{
  struct dps310_device dev;
  int32_t temp_mc, pressure;
  int ret;
  int i;

  ret = dps310_init(&dev, I2C_DEV_PATH, DPS310_ADDR_DEFAULT);
  if (ret < 0) { printf("ERROR: init failed\n"); return -1; }

  ret = dps310_probe(&dev);
  if (ret < 0) { printf("ERROR: probe failed\n"); dps310_deinit(&dev); return -1; }

  printf("[DPS310] addr=0x%02X probe OK\n", DPS310_ADDR_DEFAULT);

  dps310_reset(&dev);

  ret = dps310_read_calibration(&dev);
  if (ret < 0) { printf("ERROR: calib failed\n"); dps310_deinit(&dev); return -1; }

  for (i = 0; i < 5; i++)
    {
      ret = dps310_read_temperature(&dev, &temp_mc);
      if (ret < 0) { printf("ERROR: temp failed\n"); break; }

      ret = dps310_read_pressure(&dev, &pressure);
      if (ret < 0) { printf("ERROR: pressure failed\n"); break; }

      printf("[DPS310] sample=%d temp=%d.%02d C pressure=%d.%02d Pa\n",
             i + 1,
             (int)(temp_mc / 1000),
             (int)((temp_mc >= 0 ? temp_mc : -temp_mc) % 1000 / 10),
             (int)(pressure / 100),
             (int)((pressure >= 0 ? pressure : -pressure) % 100));

      PrivTaskDelay(1000);
    }

  dps310_deinit(&dev);
  return 0;
}
