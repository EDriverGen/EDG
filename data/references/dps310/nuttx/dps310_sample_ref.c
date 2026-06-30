/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * DPS310 Digital Pressure Sensor Sample for NuttX
 */
#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "dps310_ref.h"

extern FAR struct i2c_master_s *board_i2cbus_initialize(int bus);

int dps310_sample_main(int argc, FAR char *argv[])
{
  struct dps310_device dev;
  FAR struct i2c_master_s *i2c;
  int32_t temp_mc, pressure;
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

  ret = dps310_init(&dev, i2c, DPS310_ADDR_DEFAULT);
  if (ret < 0)
    {
      printf("ERROR: dps310_init failed: %d\n", ret);
      return ret;
    }

  ret = dps310_probe(&dev);
  if (ret < 0)
    {
      printf("ERROR: DPS310 not found at 0x%02X: %d\n",
             DPS310_ADDR_DEFAULT, ret);
      return ret;
    }

  printf("[DPS310] addr=0x%02X probe OK\n", DPS310_ADDR_DEFAULT);

  ret = dps310_reset(&dev);
  if (ret < 0)
    {
      printf("WARNING: reset failed: %d\n", ret);
    }

  ret = dps310_read_calibration(&dev);
  if (ret < 0)
    {
      printf("ERROR: read calibration failed: %d\n", ret);
      return ret;
    }

  for (i = 0; i < count; i++)
    {
      ret = dps310_read_temperature(&dev, &temp_mc);
      if (ret < 0)
        {
          printf("ERROR: read temperature failed: %d\n", ret);
          return ret;
        }

      ret = dps310_read_pressure(&dev, &pressure);
      if (ret < 0)
        {
          printf("ERROR: read pressure failed: %d\n", ret);
          return ret;
        }

      printf("[DPS310] sample=%d temp=%d.%02d C pressure=%d.%02d Pa\n",
             i + 1,
             (int)(temp_mc / 1000),
             (int)((temp_mc >= 0 ? temp_mc : -temp_mc) % 1000 / 10),
             (int)(pressure / 100),
             (int)((pressure >= 0 ? pressure : -pressure) % 100));

      if (i + 1 < count)
        {
          usleep(1000000);
        }
    }

  return 0;
}
