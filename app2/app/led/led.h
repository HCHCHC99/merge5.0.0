/**
 * @file led.h
 * @brief LED模块对外公共接口
 * @version 2.0
 */

#ifndef __LED_H__
#define __LED_H__

#include <stdint.h>
#include <stdbool.h>
#include "led_config.h"

#ifdef __cplusplus
extern "C" {
#endif

// ==============================
// 颜色定义（三基色组合）
// ==============================
typedef enum {
    LED_COLOR_BLACK        = 0x00,  // 全灭
    LED_COLOR_RED          = 0x01,  // 0b001
    LED_COLOR_GREEN        = 0x02,  // 0b010
    LED_COLOR_YELLOW       = 0x03,  // 0b011
	LED_COLOR_RED_YELLOW   = 0x04,
	LED_COLOR_RED_GREEN    = 0x05,
	LED_COLOR_GREEN_YELLOW = 0x06,
    LED_COLOR_MAX
} LED_ColorCode_t;

// ==============================
// 调度条件类型
// ==============================
typedef enum {
    LED_SCHEDULE_INFINITE = 0,   // 无限循环
    LED_SCHEDULE_COUNT,          // 按次数
    LED_SCHEDULE_TIME,           // 按时间
} LED_ScheduleType_t;

// ==============================
// 数据结构
// ==============================

// 颜色映射配置（三基色实际引脚映射）
typedef struct {
    uint8_t r_enable : 1;
    uint8_t g_enable : 1;
    uint8_t y_enable : 1;
} LED_ColorMap_t;

// 闪烁序列项（一个时间片）
typedef struct {
    uint16_t duration_ms;        // 持续时间（毫秒）
    LED_ColorCode_t color;       // 该时间段显示的颜色
} LED_SequenceItem_t;

// 闪烁配置表（完整的一个闪烁周期）
typedef struct {
    const LED_SequenceItem_t *items;  // 序列项数组指针（const，在ROM中）
    uint8_t item_count;               // 序列项数量
    uint16_t total_duration_ms;       // 总周期时间（自动计算）
    const char *name;                 // 序列名称
} LED_Sequence_t;

// 调度条件
typedef struct {
    LED_ScheduleType_t type;
    union {
        uint32_t count;          // 次数（SCHEDULE_COUNT）
        uint32_t time_ms;        // 时间毫秒（SCHEDULE_TIME）
    } param;
} LED_ScheduleParam_t;

// LED运行配置（完整的配置快照）
typedef struct {
    const LED_Sequence_t *sequence;  // 闪烁序列（const，指向ROM）
    LED_ScheduleParam_t schedule;    // 调度参数
} LED_RunConfig_t;

// 硬件配置
typedef struct {
    uint16_t pin_r;
    uint16_t pin_g;
    uint16_t pin_y;
    uint8_t gpio_r;
    uint8_t gpio_g;
    uint8_t gpio_y;
    uint8_t active_level;        // 0:低电平有效 1:高电平有效
} LED_HWConfig_t;

// LED对象（内部状态，外部不访问）
typedef struct LED_Object LED_Object_t;

// ==============================
// 外部变量声明
// ==============================
extern const uint8_t LED_COUNT;
extern const LED_HWConfig_t LED_HW_TABLE[];

// ==============================
// 公共API函数（供其他模块调用）
// ==============================

/**
 * @brief LED模块初始化
 */
void LED_Init(void);

/**
 * @brief LED模块任务（由调度器周期性调用）
 */
void LED_Task(void);

/**
 * @brief 设置LED事件（通过事件驱动）
 * @param led_idx LED索引
 * @param evt_bit 事件位（见led_events.h）
 */
void led_set_event(uint8_t led_idx, uint32_t evt_bit);

/**
 * @brief 应用新的LED运行配置
 * @param led_idx LED索引
 * @param config 新配置指针
 * @return true:成功 false:失败
 */
bool LED_ApplyConfig(uint8_t led_idx, const LED_RunConfig_t *config);

/**
 * @brief 停止LED运行
 * @param led_idx LED索引
 */
void LED_Stop(uint8_t led_idx);

/**
 * @brief 暂停LED（保持当前状态）
 * @param led_idx LED索引
 */
void LED_Pause(uint8_t led_idx);

/**
 * @brief 恢复LED运行
 * @param led_idx LED索引
 */
void LED_Resume(uint8_t led_idx);

/**
 * @brief 获取LED运行状态
 * @param led_idx LED索引
 * @return true:运行中 false:已停止
 */
bool LED_IsRunning(uint8_t led_idx);

/**
 * @brief 获取LED使能状态
 * @param led_idx LED索引
 * @return true:已使能 false:已失能
 */
bool LED_IsEnabled(uint8_t led_idx);

/**
 * @brief 直接设置LED颜色（调试用）
 * @param led_idx LED索引
 * @param color 颜色码
 */
void LED_SetColor(uint8_t led_idx, LED_ColorCode_t color);

/**
 * @brief 获取当前颜色
 * @param led_idx LED索引
 * @return 当前颜色码
 */
LED_ColorCode_t LED_GetCurrentColor(uint8_t led_idx);

/**
 * @brief 获取颜色名称字符串
 * @param color 颜色码
 * @return 颜色名称字符串
 */
const char* LED_GetColorName(LED_ColorCode_t color);

#ifdef __cplusplus
}
#endif

#endif /* __LED_H__ */
