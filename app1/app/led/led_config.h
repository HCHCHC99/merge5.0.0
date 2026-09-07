/**
 * @file led_config.h
 * @brief LED模块配置类型定义
 */

#ifndef __LED_CONFIG_H__
#define __LED_CONFIG_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ==============================
// LED数量配置
// ==============================
#define LED_COUNT_CONFIG    2

// ==============================
// 默认调度配置
// ==============================
#define LED_DEFAULT_SCHEDULE_TYPE   LED_SCHEDULE_INFINITE

// ==============================
// 任务周期（毫秒）
// ==============================
#define LED_TASK_PERIOD_MS          10

// ==============================
// 配置命令状态
// ==============================
typedef enum {
    LED_CFG_IDLE = 0,       // 空闲
    LED_CFG_PENDING,        // 待应用
    LED_CFG_APPLYING,       // 正在应用
    LED_CFG_APPLIED,        // 已应用
} LED_ConfigState_t;

#ifdef __cplusplus
}
#endif

#endif /* __LED_CONFIG_H__ */
