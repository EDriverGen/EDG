/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * TMP421 Remote Temperature Sensor Sample for XiUOS
 */
#include <stdio.h>
#include <transform.h>
#include "tmp421_ref.h"

#define I2C_DEV_PATH  "/dev/i2c1"

int tmp421_sample(void)
{
  struct tmp421_device dev;
  int32_t local_mc, remote_mc;
  int ret;
  int i;

  ret = tmp421_init(&dev, I2C_DEV_PATH, TMP421_ADDR_DEFAULT);
  if (ret < 0) { printf("ERROR: init failed\n"); return -1; }

  ret = tmp421_probe(&dev);
  if (ret < 0) { printf("ERROR: probe failed\n"); tmp421_deinit(&dev); return -1; }

  printf("[TMP421] addr=0x%02X probe OK\n", TMP421_ADDR_DEFAULT);

  for (i = 0; i < 5; i++)
    {
      ret = tmp421_read_local_temp(&dev, &local_mc);
      if (ret < 0) { printf("ERROR: local read failed\n"); break; }

      ret = tmp421_read_remote_temp(&dev, &remote_mc);
      if (ret < 0) { printf("ERROR: remote read failed\n"); break; }

      printf("[TMP421] sample=%d local=%d.%03d C remote=%d.%03d C\n",
             i + 1,
             (int)(local_mc / 1000),
             (int)(local_mc >= 0 ? local_mc % 1000 : (-local_mc) % 1000),
             (int)(remote_mc / 1000),
             (int)(remote_mc >= 0 ? remote_mc % 1000 : (-remote_mc) % 1000));

      PrivTaskDelay(1000);
    }

  tmp421_deinit(&dev);
  return 0;
}
