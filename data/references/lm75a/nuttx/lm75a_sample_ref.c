/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * LM75A Temperature Sensor Sample for NuttX
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-04-02     Lin          NuttX reference sample
 */
#include <nuttx/config.h>
#include <stdio.h>
#include <unistd.h>
#include "lm75a_ref.h"

/* Platform-specific: provided by board initialization code */
extern FAR struct i2c_master_s *board_i2cbus_initialize(int bus);

/*
 * This sample expects:
 *   - An I2C bus already initialized (e.g. board_i2c_initialize())
 *   - LM75A connected at address 0x48
 *
 * Usage:
 *   lm75a_sample [count]
 */
int lm75a_sample_main(int argc, FAR char *argv[])
{
  struct lm75a_device dev;
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

  /*
   * Get I2C bus instance.
   * Replace with your board's I2C init function, e.g.:
   *   stm32_i2cbus_initialize(1)  -- STM32
   *   sam_i2c_master_initialize(1) -- SAM
   */

  i2c = board_i2cbus_initialize(1);
  if (i2c == NULL)
    {
      printf("ERROR: Failed to get I2C bus 1\n");
      return -1;
    }

  ret = lm75a_init(&dev, i2c, LM75A_DEFAULT_ADDR);
  if (ret < 0)
    {
      printf("ERROR: lm75a_init failed: %d\n", ret);
      return ret;
    }

  ret = lm75a_probe(&dev);
  if (ret < 0)
    {
      printf("ERROR: LM75A not found at 0x%02X: %d\n",
             LM75A_DEFAULT_ADDR, ret);
      return ret;
    }

  printf("[LM75A] addr=0x%02X probe OK\n", LM75A_DEFAULT_ADDR);

  for (i = 0; i < count; i++)
    {
      ret = lm75a_read_temperature(&dev, &temp_mc);
      if (ret < 0)
        {
          printf("ERROR: read temperature failed: %d\n", ret);
          return ret;
        }

      printf("[LM75A] sample=%d temp=%d.%03d C\n",
             i + 1,
             (int)(temp_mc / 1000),
             (int)(temp_mc >= 0 ? temp_mc % 1000 : (-temp_mc) % 1000));

      if (i + 1 < count)
        {
          usleep(1000000); /* 1 second */
        }
    }

  return 0;
}
