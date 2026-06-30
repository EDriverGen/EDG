/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * LSM303DLHC Sample for NuttX
 */
#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "lsm303dlhc_ref.h"

extern FAR struct i2c_master_s *board_i2cbus_initialize(int bus);

int lsm303dlhc_sample_main(int argc, FAR char *argv[])
{
  struct lsm303dlhc_device dev;
  struct lsm303dlhc_accel_data accel;
  struct lsm303dlhc_mag_data mag;
  FAR struct i2c_master_s *i2c;
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

  ret = lsm303dlhc_init(&dev, i2c);
  if (ret < 0)
    {
      printf("ERROR: lsm303dlhc_init failed: %d\n", ret);
      return ret;
    }

  ret = lsm303dlhc_probe(&dev);
  if (ret < 0)
    {
      printf("ERROR: LSM303DLHC not found: %d\n", ret);
      return ret;
    }

  printf("[LSM303DLHC] accel=0x%02X mag=0x%02X probe OK\n",
         LSM303DLHC_ACCEL_ADDR, LSM303DLHC_MAG_ADDR);

  ret = lsm303dlhc_accel_start(&dev);
  if (ret < 0)
    {
      printf("ERROR: accel_start failed: %d\n", ret);
      return ret;
    }

  ret = lsm303dlhc_mag_start(&dev);
  if (ret < 0)
    {
      printf("ERROR: mag_start failed: %d\n", ret);
      return ret;
    }

  usleep(100000); /* wait 100 ms for first samples */

  for (i = 0; i < count; i++)
    {
      ret = lsm303dlhc_read_accel(&dev, &accel);
      if (ret < 0)
        {
          printf("ERROR: read accel failed: %d\n", ret);
          return ret;
        }

      ret = lsm303dlhc_read_mag(&dev, &mag);
      if (ret < 0)
        {
          printf("ERROR: read mag failed: %d\n", ret);
          return ret;
        }

      printf("[LSM303DLHC] sample=%d accel=(%d,%d,%d) mag=(%d,%d,%d)\n",
             i + 1,
             accel.x, accel.y, accel.z,
             mag.x, mag.y, mag.z);

      if (i + 1 < count)
        {
          usleep(500000);
        }
    }

  return 0;
}
