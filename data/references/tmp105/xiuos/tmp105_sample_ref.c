/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * TMP105 Temperature Sensor Sample for XiUOS
 */
#include <stdio.h>
#include <transform.h>
#include "tmp105_ref.h"

#define I2C_DEV_PATH  "/dev/i2c1"

int tmp105_sample(void)
{
  struct tmp105_device dev;
  int32_t temp_mc;
  int ret;
  int i;

  ret = tmp105_init(&dev, I2C_DEV_PATH, TMP105_ADDR_DEFAULT);
  if (ret < 0) { printf("ERROR: init failed\n"); return -1; }

  ret = tmp105_probe(&dev);
  if (ret < 0) { printf("ERROR: probe failed\n"); tmp105_deinit(&dev); return -1; }

  printf("[TMP105] addr=0x%02X probe OK\n", TMP105_ADDR_DEFAULT);

  tmp105_set_resolution(&dev, TMP105_RES_12BIT);

  for (i = 0; i < 5; i++)
    {
      ret = tmp105_read_temperature(&dev, &temp_mc);
      if (ret < 0) { printf("ERROR: read failed\n"); break; }

      printf("[TMP105] sample=%d temp=%d.%03d C\n",
             i + 1,
             (int)(temp_mc / 1000),
             (int)(temp_mc >= 0 ? temp_mc % 1000 : (-temp_mc) % 1000));

      PrivTaskDelay(1000);
    }

  tmp105_deinit(&dev);
  return 0;
}
