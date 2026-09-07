/**
 * @file led_module.h
 * @brief LED模块内部定义
 */

#ifndef __LED_MODULE_H__
#define __LED_MODULE_H__

#include "led.h"
#include "led_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// ==============================
// LED对象定义（内部使用）
// ==============================
struct LED_Object {
    // 硬件映射
    LED_HWConfig_t hw;

    // 当前运行配置（活动配置）
    LED_RunConfig_t active_config;

    // 待应用配置（配置命令暂存）
    LED_RunConfig_t pending_config;
    LED_ConfigState_t config_state;

    // 运行状态
    uint8_t current_item_index;
    uint32_t item_start_time;
    uint32_t schedule_current_count;
    uint32_t schedule_elapsed_time_ms;

    // 控制标志
    bool enabled;
    bool is_running;
    bool pause_requested;

    // 当前实际输出的颜色（用于调试）
    LED_ColorCode_t current_color;
};

// ==============================
// 内部函数声明
// ==============================

/**
 * @brief 内部：应用待处理配置
 * @param led LED对象指针
 * @return true:成功 false:失败
 */
bool led_module_apply_pending(LED_Object_t *led);

/**
 * @brief 内部：启动运行
 * @param led LED对象指针
 */
void led_module_start(LED_Object_t *led);

/**
 * @brief 内部：停止运行
 * @param led LED对象指针
 */
void led_module_stop(LED_Object_t *led);

/**
 * @brief 内部：更新LED硬件输出
 * @param led LED对象指针
 * @param color 颜色码
 */
void led_module_update_hw(LED_Object_t *led, LED_ColorCode_t color);

/**
 * @brief 内部：切换序列项
 * @param led LED对象指针
 * @return true:继续运行 false:停止
 */
bool led_module_next_item(LED_Object_t *led);

/**
 * @brief 内部：序列模块初始化
 */
void LED_Seq_Init(void);

/**
 * @brief GPIO初始化
 */
void led_hw_init(LED_Object_t *led);

#ifdef __cplusplus
}
#endif

#endif /* __LED_MODULE_H__ */
