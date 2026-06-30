/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-04-02     Lin          add DPS310 driver with standard structure
 */
#ifndef DRIVERS_INCLUDE_DPS310_H_
#define DRIVERS_INCLUDE_DPS310_H_

#include <rtthread.h>
#include <rtdevice.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * DPS310 的 7 位 I2C 地址由 SDO 引脚决定：
 *   SDO = GND -> 0x76
 *   SDO = V+  -> 0x77
 */
#define DPS310_ADDR_LOW              0x76
#define DPS310_ADDR_HIGH             0x77

#define DPS310_DEFAULT_BUS_NAME      "i2c1"
#define DPS310_DEFAULT_ADDR          DPS310_ADDR_HIGH

/*
 * DPS310 寄存器定义（来自数据手册 Section 7）。
 */
#define DPS310_REG_PSR_B2            0x00  /* 气压数据 [23:16] */
#define DPS310_REG_PSR_B1            0x01  /* 气压数据 [15:8]  */
#define DPS310_REG_PSR_B0            0x02  /* 气压数据 [7:0]   */
#define DPS310_REG_TMP_B2            0x03  /* 温度数据 [23:16] */
#define DPS310_REG_TMP_B1            0x04  /* 温度数据 [15:8]  */
#define DPS310_REG_TMP_B0            0x05  /* 温度数据 [7:0]   */
#define DPS310_REG_PRS_CFG           0x06  /* 气压测量配置 */
#define DPS310_REG_TMP_CFG           0x07  /* 温度测量配置 */
#define DPS310_REG_MEAS_CFG          0x08  /* 传感器操作模式和状态 */
#define DPS310_REG_CFG_REG           0x09  /* 中断和 FIFO 配置 */
#define DPS310_REG_INT_STS           0x0A  /* 中断状态 */
#define DPS310_REG_FIFO_STS          0x0B  /* FIFO 状态 */
#define DPS310_REG_RESET             0x0C  /* 软复位 */
#define DPS310_REG_PRODUCT_ID        0x0D  /* 产品和版本 ID */
#define DPS310_REG_COEF              0x10  /* 校准系数起始地址 (0x10~0x21) */
#define DPS310_REG_COEF_SRCE         0x28  /* 温度校准系数来源 */

/* MEAS_CFG 寄存器位 */
#define DPS310_MEAS_CFG_PRS_RDY      (1U << 4)
#define DPS310_MEAS_CFG_TMP_RDY      (1U << 5)
#define DPS310_MEAS_CFG_SENSOR_RDY   (1U << 6)
#define DPS310_MEAS_CFG_COEF_RDY     (1U << 7)

/* 测量模式 (MEAS_CFG[2:0]) */
#define DPS310_MODE_IDLE             0x00
#define DPS310_MODE_PRS_SINGLE       0x01
#define DPS310_MODE_TMP_SINGLE       0x02
#define DPS310_MODE_PRS_CONT         0x05
#define DPS310_MODE_TMP_CONT         0x06
#define DPS310_MODE_PRS_TMP_CONT     0x07

/* 软复位命令 */
#define DPS310_RESET_SOFT            0x89

/* 产品 ID 预期值（REV_ID | PROD_ID) */
#define DPS310_PRODUCT_ID            0x10

/*
 * 过采样率对应的缩放因子。
 * 来自数据手册 Section 4.9.1。
 */
#define DPS310_SCALE_FACTOR_1        524288
#define DPS310_SCALE_FACTOR_2        1572864
#define DPS310_SCALE_FACTOR_4        3670016
#define DPS310_SCALE_FACTOR_8        7864320
#define DPS310_SCALE_FACTOR_16       253952
#define DPS310_SCALE_FACTOR_32       516096
#define DPS310_SCALE_FACTOR_64       1040384
#define DPS310_SCALE_FACTOR_128      2088960

/*
 * DPS310 校准系数结构体。
 * 上电后从芯片 NVRAM 读取一次即可。
 */
struct dps310_calib_coeff
{
    rt_int32_t c0;
    rt_int32_t c1;
    rt_int32_t c00;
    rt_int32_t c10;
    rt_int32_t c01;
    rt_int32_t c11;
    rt_int32_t c20;
    rt_int32_t c21;
    rt_int32_t c30;
};

/*
 * DPS310 驱动设备对象。
 */
struct dps310_device
{
    struct rt_i2c_bus_device *bus;
    const char *bus_name;
    rt_uint8_t addr;
    struct dps310_calib_coeff coeff;
    rt_int32_t kT;  /* 温度缩放因子 */
    rt_int32_t kP;  /* 气压缩放因子 */
};

/*
 * 初始化 DPS310 设备对象。
 */
rt_err_t dps310_init(struct dps310_device *dev,
                     const char           *bus_name,
                     rt_uint8_t            addr);

/*
 * 探测 DPS310 是否在线（读取产品 ID）。
 */
rt_err_t dps310_probe(struct dps310_device *dev);

/*
 * 软复位。
 */
rt_err_t dps310_reset(struct dps310_device *dev);

/*
 * 读取校准系数（初始化后应调用一次）。
 */
rt_err_t dps310_read_calibration(struct dps310_device *dev);

/*
 * 触发一次温度测量并读取结果（摄氏度 × 100）。
 */
rt_err_t dps310_read_temperature(struct dps310_device *dev,
                                 rt_int32_t           *temp_c100);

/*
 * 触发一次气压测量并读取结果（Pa × 100）。
 * 需要先做一次温度测量以补偿。
 */
rt_err_t dps310_read_pressure(struct dps310_device *dev,
                              rt_int32_t           *pressure_pa100);

#ifdef __cplusplus
}
#endif

#endif /* DRIVERS_INCLUDE_DPS310_H_ */
