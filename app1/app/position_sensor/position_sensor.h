/**
 * @file position_sensor.h
 * @brief 推杆位置传感器统一抽象接口
 * @note 支持多传感器自动注册 + 可裁剪编译
 */

#ifndef __POSITION_SENSOR_H
#define __POSITION_SENSOR_H

#include "sys_config.h"
#include <stdint.h>

/**
 * @brief 位置传感器类型枚举
 */
typedef enum {
    POS_SENSOR_NONE         = 0,
    POS_SENSOR_POT          = 1,    /* 电位计 */
    POS_SENSOR_HALL         = 2,    /* 电机霍尔 */
    POS_SENSOR_INC_ENC      = 3,    /* 增量编码器 */
    POS_SENSOR_ABS_ENC      = 4,    /* 绝对编码器 */
} PosSensorType_t;

/**
 * @brief 统一位置数据结构
 */
typedef struct {
    int32_t  raw;                /* 原始采样值 */
    int32_t  mm;                 /* 实际位置 mm */
    float    percent;            /* 行程百分比 0~100% */
    uint8_t  valid;              /* 数据有效标志 */
} PosData_t;

/**
 * @brief 传感器统一操作接口
 */
typedef struct {
    uint8_t  (*init)(void);
    int32_t  (*read_raw)(void);
    int32_t  (*read_mm)(void);
    float    (*read_percent)(void);
    void     (*calibrate)(void);
} PosSensorOps_t;

/**
 * @brief 传感器注册结构体
 */
typedef struct {
    PosSensorType_t    type;
    const PosSensorOps_t *ops;
    const char         *name;
} PosSensorRegister_t;

/* 对外接口 */
void                PosSensor_Init(void);
void                PosSensor_Task(void);
const PosData_t*    PosSensor_GetData(void);

#endif /* __POSITION_SENSOR_H */

