/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-04-02     Lin          add LSM303DLHC driver with standard structure
 */
#ifndef DRIVERS_INCLUDE_LSM303DLHC_H_
#define DRIVERS_INCLUDE_LSM303DLHC_H_

#include <rtthread.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * LSM303DLHC 有两个独立的 I2C 从机地址（加速度计和磁力计分开）。
 * 地址固定，不可由外部引脚配置。
 */
#define LSM303DLHC_ACCEL_ADDR        0x19   /* 加速度计 I2C 地址 */
#define LSM303DLHC_MAG_ADDR          0x1E   /* 磁力计 I2C 地址 */

#define LSM303DLHC_DEFAULT_BUS_NAME  "i2c1"

/* ---- 加速度计寄存器 ---- */
#define LSM303DLHC_CTRL_REG1_A       0x20
#define LSM303DLHC_CTRL_REG4_A       0x23
#define LSM303DLHC_STATUS_REG_A      0x27
#define LSM303DLHC_OUT_X_L_A         0x28
#define LSM303DLHC_OUT_X_H_A         0x29
#define LSM303DLHC_OUT_Y_L_A         0x2A
#define LSM303DLHC_OUT_Y_H_A         0x2B
#define LSM303DLHC_OUT_Z_L_A         0x2C
#define LSM303DLHC_OUT_Z_H_A         0x2D

/* CTRL_REG1_A 位定义 */
#define LSM303DLHC_ODR_1HZ           0x10
#define LSM303DLHC_ODR_10HZ          0x20
#define LSM303DLHC_ODR_25HZ          0x30
#define LSM303DLHC_ODR_50HZ          0x40
#define LSM303DLHC_ODR_100HZ         0x50
#define LSM303DLHC_AXES_ENABLE       0x07   /* 使能 X/Y/Z 三轴 */

/* CTRL_REG4_A 位定义 */
#define LSM303DLHC_FS_2G             0x00
#define LSM303DLHC_FS_4G             0x10
#define LSM303DLHC_FS_8G             0x20
#define LSM303DLHC_FS_16G            0x30
#define LSM303DLHC_HR_BIT            0x08   /* 高分辨率模式 */

/* ---- 磁力计寄存器 ---- */
#define LSM303DLHC_CRA_REG_M         0x00   /* 数据输出速率 */
#define LSM303DLHC_CRB_REG_M         0x01   /* 增益配置 */
#define LSM303DLHC_MR_REG_M          0x02   /* 模式寄存器 */
#define LSM303DLHC_OUT_X_H_M         0x03
#define LSM303DLHC_OUT_X_L_M         0x04
#define LSM303DLHC_OUT_Z_H_M         0x05
#define LSM303DLHC_OUT_Z_L_M         0x06
#define LSM303DLHC_OUT_Y_H_M         0x07
#define LSM303DLHC_OUT_Y_L_M         0x08
#define LSM303DLHC_SR_REG_M          0x09   /* 状态寄存器 */
#define LSM303DLHC_IRA_REG_M         0x0A   /* 识别寄存器 A: 0x48 */
#define LSM303DLHC_IRB_REG_M         0x0B   /* 识别寄存器 B: 0x34 */
#define LSM303DLHC_IRC_REG_M         0x0C   /* 识别寄存器 C: 0x33 */

/* 磁力计模式 */
#define LSM303DLHC_MAG_CONTINUOUS    0x00
#define LSM303DLHC_MAG_SINGLE        0x01
#define LSM303DLHC_MAG_SLEEP         0x03

/* 磁力计增益 */
#define LSM303DLHC_MAG_GAIN_1_3      0x20   /* ±1.3 gauss, 1100 LSB/gauss */
#define LSM303DLHC_MAG_GAIN_1_9      0x40
#define LSM303DLHC_MAG_GAIN_4_0      0xC0

/* 磁力计数据速率 */
#define LSM303DLHC_MAG_ODR_15HZ      0x10
#define LSM303DLHC_MAG_ODR_30HZ      0x14
#define LSM303DLHC_MAG_ODR_75HZ      0x18

/* 识别寄存器预期值 */
#define LSM303DLHC_IRA_VALUE         0x48
#define LSM303DLHC_IRB_VALUE         0x34
#define LSM303DLHC_IRC_VALUE         0x33

/*
 * 三轴数据结构。
 */
struct lsm303dlhc_xyz
{
    rt_int16_t x;
    rt_int16_t y;
    rt_int16_t z;
};

/*
 * LSM303DLHC 驱动设备对象。
 */
struct lsm303dlhc_device
{
    struct rt_i2c_bus_device *bus;
    const char *bus_name;
};

/*
 * 初始化设备对象。
 */
rt_err_t lsm303dlhc_init(struct lsm303dlhc_device *dev,
                          const char               *bus_name);

/*
 * 探测磁力计是否在线（读取识别寄存器）。
 */
rt_err_t lsm303dlhc_probe(struct lsm303dlhc_device *dev);

/*
 * 配置并启动加速度计（默认 50Hz, ±2g, 高分辨率）。
 */
rt_err_t lsm303dlhc_accel_start(struct lsm303dlhc_device *dev);

/*
 * 读取加速度计原始值。
 */
rt_err_t lsm303dlhc_accel_read_raw(struct lsm303dlhc_device *dev,
                                    struct lsm303dlhc_xyz    *accel);

/*
 * 配置并启动磁力计（默认连续模式, 15Hz, ±1.3 gauss）。
 */
rt_err_t lsm303dlhc_mag_start(struct lsm303dlhc_device *dev);

/*
 * 读取磁力计原始值。
 */
rt_err_t lsm303dlhc_mag_read_raw(struct lsm303dlhc_device *dev,
                                  struct lsm303dlhc_xyz    *mag);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_INCLUDE_LSM303DLHC_H_ */
