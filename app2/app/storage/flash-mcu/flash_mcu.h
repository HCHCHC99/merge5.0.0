
/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-03-27     gylocal       the first version
 * 2026-07-14     gylocal       优化：打通work_config、位置限幅、自动计算结合点
 */
#ifndef _FLASH_MCU_H_
#define _FLASH_MCU_H_

#include <stdint.h>
#include "work_config.h"

typedef struct storage storage_t;

typedef enum
{
    AXIS_NUM_0 = 0,
    AXIS_NUM_1,
    AXIS_NUM_MAX
}axis_num_t;

// 底层私有读写操作
typedef struct
{
    int32_t (*load)(storage_t *self);
    int32_t (*save)(storage_t *self);
} storage_priv_ops_t;

// 轴存储数据结构：仅存储分离点，结合点运行时计算
typedef struct
{
    uint32_t calib_flag;
    float position_combine;    // 运行时计算，写入Flash
    float position_separae; // 采样分离点，落地存储
}axis_data_t;

// Flash持久化私有存储块
typedef struct
{
    uint32_t  magic;
    Work_Config_t work_config;  // 完整机型配置，与work_config模块互通
    uint16_t  soft_vn;          // 软件版本
    axis_data_t axis_data[AXIS_NUM_MAX];
} priv_data_t;

// 对外存储操作接口
typedef struct
{
    // 存储初始化
    int32_t  (*init)(storage_t *self);

    // 机型配置读写（完整Work_Config_t，直接对接work_config）
    int32_t  (*write_model)(storage_t *self, Work_Config_t cfg);
    Work_Config_t (*read_model)(storage_t *self);

    // 软件版本读写
    int32_t  (*write_vn)(storage_t *self, uint16_t vn);
    uint16_t (*read_vn)(storage_t *self);

    // 标定标志读写
    int32_t  (*write_calib)(storage_t *self, axis_num_t index, uint32_t flag);
    uint32_t (*read_calib)(storage_t *self, axis_num_t index);

    // 分离点位置读写（带边界校验）
    int32_t  (*write_pos_separate)(storage_t *self, axis_num_t index, float pos);
    float (*read_pos_separate)(storage_t *self, axis_num_t index);

    float (*read_pos_combine)(storage_t *self, axis_num_t index);

    // 读取全部存储原始数据
    priv_data_t   (*read_all)(storage_t *self);

} storage_ops_t;

// 存储实例对象
struct storage
{
    priv_data_t data;
    storage_ops_t *ops;
    storage_priv_ops_t *priv_ops;
};

extern storage_t storage_dev;

// 上层BSP封装接口
void bsp_storeage_init(void);

// 机型配置
int32_t bsp_write_model(Work_Config_t cfg);
Work_Config_t bsp_read_model(void);

// 软件版本
int32_t bsp_write_vn(uint16_t vn);
uint16_t bsp_read_vn(void);

// 标定标志
int32_t bsp_write_cali(axis_num_t index ,uint32_t flag);
uint32_t bsp_read_cali(axis_num_t index);

// 分离点存储
int32_t bsp_write_pos_separate(axis_num_t index, float pos);
float bsp_read_pos_separate(axis_num_t index);

float bsp_read_pos_combine(axis_num_t index);

// 读取全部存储原始数据
priv_data_t bsp_read_all(void);

#endif /* _FLASH_MCU_H_ */
