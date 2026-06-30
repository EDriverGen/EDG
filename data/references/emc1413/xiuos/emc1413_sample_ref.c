/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * EMC1413 3-Channel Temperature Sensor Sample for XiUOS
 */
#include <stdio.h>
#include <transform.h>
#include "emc1413_ref.h"

#define I2C_DEV_PATH  "/dev/i2c1"

int emc1413_sample(void)
{
  struct emc1413_device dev;
  int32_t internal_mc, ext1_mc, ext2_mc;
  int ret;
  int i;

  ret = emc1413_init(&dev, I2C_DEV_PATH, EMC1413_ADDR_DEFAULT);
  if (ret < 0) { printf("ERROR: init failed\n"); return -1; }

  ret = emc1413_probe(&dev);
  if (ret < 0) { printf("ERROR: probe failed\n"); emc1413_deinit(&dev); return -1; }

  printf("[EMC1413] addr=0x%02X probe OK\n", EMC1413_ADDR_DEFAULT);

  for (i = 0; i < 5; i++)
    {
      ret = emc1413_read_internal_temp(&dev, &internal_mc);
      if (ret < 0) { printf("ERROR: internal read failed\n"); break; }

      ret = emc1413_read_ext1_temp(&dev, &ext1_mc);
      if (ret < 0) { printf("ERROR: ext1 read failed\n"); break; }

      ret = emc1413_read_ext2_temp(&dev, &ext2_mc);
      if (ret < 0) { printf("ERROR: ext2 read failed\n"); break; }

      printf("[EMC1413] sample=%d internal=%d.%03d C ext1=%d.%03d C ext2=%d.%03d C\n",
             i + 1,
             (int)(internal_mc / 1000),
             (int)(internal_mc >= 0 ? internal_mc % 1000 : (-internal_mc) % 1000),
             (int)(ext1_mc / 1000),
             (int)(ext1_mc >= 0 ? ext1_mc % 1000 : (-ext1_mc) % 1000),
             (int)(ext2_mc / 1000),
             (int)(ext2_mc >= 0 ? ext2_mc % 1000 : (-ext2_mc) % 1000));

      PrivTaskDelay(1000);
    }

  emc1413_deinit(&dev);
  return 0;
}
