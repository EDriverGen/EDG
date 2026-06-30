/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * TMP105 Temperature Sensor Sample for NuttX
 */
#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "tmp105_ref.h"

extern FAR struct i2c_master_s *board_i2cbus_initialize(int bus);

int tmp105_sample_main(int argc, FAR char *argv[])
{
  struct tmp105_device dev;
  FAR struct i2c_master_s *i2c;
  int32_t temp_mc;
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

  ret = tmp105_init(&dev, i2c, TMP105_ADDR_DEFAULT);
  if (ret < 0)
    {
      printf("ERROR: tmp105_init failed: %d\n", ret);
      return ret;
    }

  ret = tmp105_probe(&dev);
  if (ret < 0)
    {
      printf("ERROR: TMP105 not found at 0x%02X: %d\n",
             TMP105_ADDR_DEFAULT, ret);
      return ret;
    }

  printf("[TMP105] addr=0x%02X probe OK\n", TMP105_ADDR_DEFAULT);

  /* Set 12-bit resolution */

  ret = tmp105_set_resolution(&dev, TMP105_RES_12BIT);
  if (ret < 0)
    {
      printf("WARNING: set resolution failed: %d\n", ret);
    }

  for (i = 0; i < count; i++)
    {
      ret = tmp105_read_temperature(&dev, &temp_mc);
      if (ret < 0)
        {
          printf("ERROR: read temperature failed: %d\n", ret);
          return ret;
        }

      printf("[TMP105] sample=%d temp=%d.%03d C\n",
             i + 1,
             (int)(temp_mc / 1000),
             (int)(temp_mc >= 0 ? temp_mc % 1000 : (-temp_mc) % 1000));

      if (i + 1 < count)
        {
          usleep(1000000);
        }
    }

  return 0;
}
