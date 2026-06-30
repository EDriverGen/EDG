/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-04-02     Lin          add TMP421 driver with standard structure
 */
#ifndef DRIVERS_INCLUDE_TMP421_H_
#define DRIVERS_INCLUDE_TMP421_H_

#include <rtthread.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * TMP421 的 7 位 I2C 地址由 A0、A1 引脚组合决定。
 * 默认 A0=GND, A1=GND 时地址为 0x2A。
 * 共 9 种可选地址：0x2A, 0x4C, 0x4D, 0x4E, 0x4F, 0x1C, 0x1D, 0x1E, 0x1F
 */
#define TMP421_ADDR_DEFAULT          0x2A

/* 默认总线名和默认地址。 */
#define TMP421_DEFAULT_BUS_NAME      "i2c1"
#define TMP421_DEFAULT_ADDR          TMP421_ADDR_DEFAULT

/*
 * TMP421 寄存器定义（只读温度寄存器 + 配置寄存器）。
 * 来自 TMP421 数据手册 Table 1。
 */
#define TMP421_REG_LOCAL_TEMP_HI     0x00  /* 本地温度高字节 */
#define TMP421_REG_LOCAL_TEMP_LO     0x10  /* 本地温度低字节 */
#define TMP421_REG_REMOTE_TEMP_HI    0x01  /* 远程通道 1 温度高字节 */
#define TMP421_REG_REMOTE_TEMP_LO    0x11  /* 远程通道 1 温度低字节 */
#define TMP421_REG_STATUS            0x08  /* 状态寄存器 */
#define TMP421_REG_CONFIG_1          0x09  /* 配置寄存器 1 */
#define TMP421_REG_CONFIG_1_WR       0x09
#define TMP421_REG_CONFIG_2          0x0A  /* 配置寄存器 2 */
#define TMP421_REG_CONFIG_2_WR       0x0A
#define TMP421_REG_CONV_RATE_RD      0x04  /* 转换速率（只读） */
#define TMP421_REG_CONV_RATE_WR      0x0B  /* 转换速率（只写） */
#define TMP421_REG_ONE_SHOT          0x0F  /* 单次触发 */
#define TMP421_REG_MANUFACTURER_ID   0xFE  /* 制造商 ID: 0x55 (TI) */
#define TMP421_REG_DEVICE_ID         0xFF  /* 设备 ID */

/* 制造商 ID 预期值 */
#define TMP421_MANUFACTURER_ID_TI    0x55

/* 配置位定义 */
#define TMP421_CONFIG1_RANGE         (1U << 2) /* 0: 0~127C, 1: -55~150C */
#define TMP421_CONFIG1_SHUTDOWN      (1U << 6) /* 1: shutdown */

/*
 * TMP421 温度：12 位分辨率，0.0625 C 步进。
 * 扩展范围模式下：-55 C ~ +150 C。
 */
#define TMP421_TEMP_MC_MIN           (-55000)
#define TMP421_TEMP_MC_MAX           150000

/*
 * TMP421 驱动设备对象。
 */
struct tmp421_device
{
    struct rt_i2c_bus_device *bus;
    const char *bus_name;
    rt_uint8_t addr;
};

/*
 * 初始化 TMP421 设备对象。
 */
rt_err_t tmp421_init(struct tmp421_device *dev,
                     const char           *bus_name,
                     rt_uint8_t            addr);

/*
 * 探测 TMP421 是否在线（读取制造商 ID 验证）。
 */
rt_err_t tmp421_probe(struct tmp421_device *dev);

/*
 * 读取本地温度，返回毫摄氏度。
 */
rt_err_t tmp421_read_local_temp(struct tmp421_device *dev,
                                rt_int32_t           *temp_mcelsius);

/*
 * 读取远程通道 1 温度，返回毫摄氏度。
 */
rt_err_t tmp421_read_remote_temp(struct tmp421_device *dev,
                                 rt_int32_t           *temp_mcelsius);

/*
 * 使能扩展温度范围（-55 ~ +150 C）。
 */
rt_err_t tmp421_set_extended_range(struct tmp421_device *dev, rt_bool_t enable);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_INCLUDE_TMP421_H_ */
