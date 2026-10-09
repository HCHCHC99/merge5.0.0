// /**
//  * @file    ctrl_current.h
//  * @brief   电流环控制（PWM中断调用）
//  */

// #ifndef __CTRL_CURRENT_H
// #define __CTRL_CURRENT_H

// #include "axis_typedef.h"

// void Ctrl_Current_Run(Axis_t *axis);

// #endif

#ifndef __CTRL_CURRENT_H
#define __CTRL_CURRENT_H

#include "axis_typedef.h"
#include "sys_sched.h"

#ifdef __cplusplus
extern "C" {
#endif

void Ctrl_Curr_Init(void);
void Ctrl_Curr_Run(void);  // PWM中断调用

//设置堵转电流
void ctrl_set_over_current(uint8_t idx, float over_current_thr);

#ifdef __cplusplus
}
#endif

#endif
