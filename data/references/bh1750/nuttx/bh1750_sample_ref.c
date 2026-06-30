/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * BH1750 Light Sensor Sample for NuttX
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-04-02     Lin          NuttX reference sample
 */
#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "bh1750_ref.h"

extern FAR struct i2c_master_s *board_i2cbus_initialize(int bus);

int bh1750_sample_main(int argc, FAR char *argv[])
{
  struct bh1750_device dev;
  FAR struct i2c_master_s *i2c;
  uint32_t lux_x100;
  int count = 5;
  int ret;
  int i;

  if (argc >= 2)
    {
      count = atoi(argv[1]);
      if (count <= 0) count = 1;
    }

  /* Board-specific: replace with your platform's I2C init function */

  i2c = board_i2cbus_initialize(1);
  if (i2c == NULL)
    {
      printf("ERROR: Failed to get I2C bus 1\n");
      return -1;
    }

  ret = bh1750_init(&dev, i2c, BH1750_DEFAULT_ADDR);
  if (ret < 0)
    {
      printf("ERROR: bh1750_init failed: %d\n", ret);
      return ret;
    }

  ret = bh1750_probe(&dev);
  if (ret < 0)
    {
      printf("ERROR: BH1750 not found at 0x%02X: %d\n",
             BH1750_DEFAULT_ADDR, ret);
      return ret;
    }

  printf("[BH1750] addr=0x%02X probe OK\n", BH1750_DEFAULT_ADDR);

  for (i = 0; i < count; i++)
    {
      ret = bh1750_read_lux_x100(&dev, &lux_x100);
      if (ret < 0)
        {
          printf("ERROR: read lux failed: %d\n", ret);
          return ret;
        }

      printf("[BH1750] sample=%d lux=%u.%02u\n",
             i + 1,
             (unsigned)(lux_x100 / 100),
             (unsigned)(lux_x100 % 100));

      if (i + 1 < count)
        {
          usleep(1000000);
        }
    }

  return 0;
}
