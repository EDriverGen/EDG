/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * EMC1413 3-Channel Temperature Sensor Sample for NuttX
 */
#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "emc1413_ref.h"

extern FAR struct i2c_master_s *board_i2cbus_initialize(int bus);

int emc1413_sample_main(int argc, FAR char *argv[])
{
  struct emc1413_device dev;
  FAR struct i2c_master_s *i2c;
  int32_t internal_mc, ext1_mc, ext2_mc;
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

  ret = emc1413_init(&dev, i2c, EMC1413_ADDR_DEFAULT);
  if (ret < 0)
    {
      printf("ERROR: emc1413_init failed: %d\n", ret);
      return ret;
    }

  ret = emc1413_probe(&dev);
  if (ret < 0)
    {
      printf("ERROR: EMC1413 not found at 0x%02X: %d\n",
             EMC1413_ADDR_DEFAULT, ret);
      return ret;
    }

  printf("[EMC1413] addr=0x%02X probe OK\n", EMC1413_ADDR_DEFAULT);

  for (i = 0; i < count; i++)
    {
      ret = emc1413_read_internal_temp(&dev, &internal_mc);
      if (ret < 0)
        {
          printf("ERROR: read internal temp failed: %d\n", ret);
          return ret;
        }

      ret = emc1413_read_ext1_temp(&dev, &ext1_mc);
      if (ret < 0)
        {
          printf("ERROR: read ext1 temp failed: %d\n", ret);
          return ret;
        }

      ret = emc1413_read_ext2_temp(&dev, &ext2_mc);
      if (ret < 0)
        {
          printf("ERROR: read ext2 temp failed: %d\n", ret);
          return ret;
        }

      printf("[EMC1413] sample=%d internal=%d.%03d C ext1=%d.%03d C ext2=%d.%03d C\n",
             i + 1,
             (int)(internal_mc / 1000),
             (int)(internal_mc >= 0 ? internal_mc % 1000 : (-internal_mc) % 1000),
             (int)(ext1_mc / 1000),
             (int)(ext1_mc >= 0 ? ext1_mc % 1000 : (-ext1_mc) % 1000),
             (int)(ext2_mc / 1000),
             (int)(ext2_mc >= 0 ? ext2_mc % 1000 : (-ext2_mc) % 1000));

      if (i + 1 < count)
        {
          usleep(1000000);
        }
    }

  return 0;
}
