/**
 * @file    pid_common.h
 * @brief   通用PID算法模块（抽象层）
 * @brief   支持位置式/增量式/积分分离/抗饱和
 * @brief   纯算法，无硬件、无状态机
 */

#ifndef __PID_COMMON_H
#define __PID_COMMON_H

#include "axis_typedef.h"

/**
 * @brief  PID初始化
 * @param  param: PID参数指针
 * @param  ctrl: PID运行数据指针
 * @return 无
 */
void PID_Init(PID_Param_t *param, PID_Ctrl_t *ctrl);

/**
 * @brief  PID统一计算接口
 * @param  param: 参数
 * @param  ctrl: 运行数据
 * @param  set: 设定值
 * @param  fdb: 反馈值
 * @return 计算输出
 */
float PID_Calc(PID_Param_t *param, PID_Ctrl_t *ctrl, float set, float fdb);

/**
 * @brief  PID复位（清空积分、误差）
 * @param  ctrl: 运行数据
 * @return 无
 */
void PID_Reset(PID_Ctrl_t *ctrl);

/**
 * @brief  设置PID输出限幅范围
 * @param  ctrl: PID运行数据句柄
 * @param  out_min: 输出最小值
 * @param  out_max: 输出最大值
 * @retval 无
 */
void PID_SetOutputLimit(PID_Ctrl_t *ctrl, float out_min, float out_max);

/**
 * @brief  单独设置PID目标设定值（仅更新，不运算）
 * @param  ctrl: PID运行数据句柄
 * @param  target: 目标设定值set
 * @retval 无
 */
void PID_SetTarget(PID_Ctrl_t *ctrl, float target);

void PID_ResetOutput(PID_Ctrl_t *ctrl);
#endif
