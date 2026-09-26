/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * BH1750 Light Sensor Sample for XiUOS
 */
#include <stdio.h>
#include <transform.h>
#include "bh1750_ref.h"

#define I2C_DEV_PATH  "/dev/i2c1"

int bh1750_sample(void)
{
  struct bh1750_device dev;
  uint32_t lux_x100;
  int ret;
  int i;

  ret = bh1750_init(&dev, I2C_DEV_PATH, BH1750_DEFAULT_ADDR);
  if (ret < 0) { printf("ERROR: init failed\n"); return -1; }

  ret = bh1750_probe(&dev);
  if (ret < 0) { printf("ERROR: probe failed\n"); bh1750_deinit(&dev); return -1; }

  printf("[BH1750] addr=0x%02X probe OK\n", BH1750_DEFAULT_ADDR);

  for (i = 0; i < 5; i++)
    {
      ret = bh1750_read_lux_x100(&dev, &lux_x100);
      if (ret < 0) { printf("ERROR: read failed\n"); break; }

      printf("[BH1750] sample=%d lux=%u.%02u\n",
             i + 1,
             (unsigned)(lux_x100 / 100),
             (unsigned)(lux_x100 % 100));

      PrivTaskDelay(1000);
    }

  bh1750_deinit(&dev);
  return 0;
}
