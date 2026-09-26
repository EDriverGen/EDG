/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * TMP421 Remote Temperature Sensor Sample for NuttX
 */
#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "tmp421_ref.h"

extern FAR struct i2c_master_s *board_i2cbus_initialize(int bus);

int tmp421_sample_main(int argc, FAR char *argv[])
{
  struct tmp421_device dev;
  FAR struct i2c_master_s *i2c;
  int32_t local_mc, remote_mc;
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

  ret = tmp421_init(&dev, i2c, TMP421_ADDR_DEFAULT);
  if (ret < 0)
    {
      printf("ERROR: tmp421_init failed: %d\n", ret);
      return ret;
    }

  ret = tmp421_probe(&dev);
  if (ret < 0)
    {
      printf("ERROR: TMP421 not found at 0x%02X: %d\n",
             TMP421_ADDR_DEFAULT, ret);
      return ret;
    }

  printf("[TMP421] addr=0x%02X probe OK\n", TMP421_ADDR_DEFAULT);

  for (i = 0; i < count; i++)
    {
      ret = tmp421_read_local_temp(&dev, &local_mc);
      if (ret < 0)
        {
          printf("ERROR: read local temp failed: %d\n", ret);
          return ret;
        }

      ret = tmp421_read_remote_temp(&dev, &remote_mc);
      if (ret < 0)
        {
          printf("ERROR: read remote temp failed: %d\n", ret);
          return ret;
        }

      printf("[TMP421] sample=%d local=%d.%03d C remote=%d.%03d C\n",
             i + 1,
             (int)(local_mc / 1000),
             (int)(local_mc >= 0 ? local_mc % 1000 : (-local_mc) % 1000),
             (int)(remote_mc / 1000),
             (int)(remote_mc >= 0 ? remote_mc % 1000 : (-remote_mc) % 1000));

      if (i + 1 < count)
        {
          usleep(1000000);
        }
    }

  return 0;
}
