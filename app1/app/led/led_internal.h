/**
 * @file led_internal.h
 * @brief LED模块内部接口（仅供模块注册使用）
 */

#ifndef __LED_INTERNAL_H__
#define __LED_INTERNAL_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// ==============================
// 模块注册函数
// ==============================

/**
 * @brief LED模块注册到系统调度器
 * @note 由系统初始化时调用
 */
void LED_Module_Register(void);

/**
 * @brief LED模块初始化（由调度器调用）
 */
void LED_Init(void);

/**
 * @brief LED模块任务（由调度器周期性调用）
 */
void LED_Task(void);

#ifdef __cplusplus
}
#endif

#endif /* __LED_INTERNAL_H__ */
