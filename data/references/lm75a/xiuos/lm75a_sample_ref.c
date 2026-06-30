/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * LM75A Temperature Sensor Sample for XiUOS
 */
#include <stdio.h>
#include <transform.h>
#include "lm75a_ref.h"

#define I2C_DEV_PATH  "/dev/i2c1"

int lm75a_sample(void)
{
  struct lm75a_device dev;
  int32_t temp_mc;
  int ret;
  int i;

  ret = lm75a_init(&dev, I2C_DEV_PATH, LM75A_DEFAULT_ADDR);
  if (ret < 0)
    {
      printf("ERROR: lm75a_init failed\n");
      return -1;
    }

  ret = lm75a_probe(&dev);
  if (ret < 0)
    {
      printf("ERROR: LM75A not found at 0x%02X\n", LM75A_DEFAULT_ADDR);
      lm75a_deinit(&dev);
      return -1;
    }

  printf("[LM75A] addr=0x%02X probe OK\n", LM75A_DEFAULT_ADDR);

  for (i = 0; i < 5; i++)
    {
      ret = lm75a_read_temperature(&dev, &temp_mc);
      if (ret < 0)
        {
          printf("ERROR: read temperature failed\n");
          break;
        }

      printf("[LM75A] sample=%d temp=%d.%03d C\n",
             i + 1,
             (int)(temp_mc / 1000),
             (int)(temp_mc >= 0 ? temp_mc % 1000 : (-temp_mc) % 1000));

      PrivTaskDelay(1000);
    }

  lm75a_deinit(&dev);
  return 0;
}
