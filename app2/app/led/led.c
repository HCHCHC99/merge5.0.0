/**
 * @file led.c
 * @brief LED对外API实现
 */

#include "led.h"
#include "led_module.h"
#include "led_sequence.h"
#include "led_seq_id.h"
#include "led_event_map.h"
#include "led_events.h"
#include "led_config.h"
#include "sys_tick.h"
#include "hc32_ll_gpio.h"
#include "hc32_ll_utility.h"

#include <string.h>
#include <stdlib.h>

// ==============================
// 外部引用
// ==============================
extern LED_Object_t g_led_array[];
extern const uint8_t LED_COUNT;
extern const LED_HWConfig_t LED_HW_TABLE[];

// ==============================
// 颜色名称表
// ==============================
static const char *g_color_names[] = {
    [LED_COLOR_BLACK]   = "Black",
    [LED_COLOR_RED]     = "Red",
    [LED_COLOR_GREEN]   = "Green",
    [LED_COLOR_YELLOW]  = "Yellow",
};

// ==============================
// 公共API实现
// ==============================

/**
 * @brief LED模块初始化
 */
void LED_Init(void)
{
    // 初始化序列（计算总时长）
    LED_Seq_Init();

    // 初始化所有LED对象
    for (uint8_t i = 0; i < LED_COUNT; i++) {
        LED_Object_t *led = &g_led_array[i];

        // 复制硬件配置
        memcpy(&led->hw, &LED_HW_TABLE[i], sizeof(LED_HWConfig_t));

        // 初始化状态
        memset(&led->active_config, 0, sizeof(LED_RunConfig_t));
        memset(&led->pending_config, 0, sizeof(LED_RunConfig_t));
        led->config_state = LED_CFG_IDLE;
        led->current_item_index = 0;
        led->item_start_time = 0;
        led->schedule_current_count = 0;
        led->schedule_elapsed_time_ms = 0;
        led->enabled = false;
        led->is_running = false;
        led->pause_requested = false;
        led->current_color = LED_COLOR_BLACK;

        // 初始化硬件
        led_hw_init(led);
        led_module_update_hw(led, LED_COLOR_BLACK);
    }
}

/**
 * @brief LED模块任务（由调度器周期性调用）
 */
void LED_Task(void)
{
    static uint32_t last_tick = 0;
    uint32_t now = SysTick_GetTick();

    // 10ms周期执行
    if (now - last_tick < LED_TASK_PERIOD_MS) return;
    last_tick = now;

    for (uint8_t i = 0; i < LED_COUNT; i++) {
        LED_Object_t *led = &g_led_array[i];

        // 检查是否有待应用配置
        if (led->config_state == LED_CFG_PENDING) {
            led_module_apply_pending(led);
        }

        // 如果未运行或暂停，跳过
        if (!led->is_running || !led->enabled || led->pause_requested) continue;
        if (!led->active_config.sequence) continue;

        // 检查当前序列项是否超时
        const LED_SequenceItem_t *item = &led->active_config.sequence->items[led->current_item_index];
        uint32_t elapsed = now - led->item_start_time;

        if (elapsed >= item->duration_ms) {
            led_module_next_item(led);
        }
    }
}

/**
 * @brief 应用新的LED运行配置
 */
bool LED_ApplyConfig(uint8_t led_idx, const LED_RunConfig_t *config)
{
    if (led_idx >= LED_COUNT || config == NULL) return false;
    if (config->sequence == NULL || config->sequence->item_count == 0) return false;

    LED_Object_t *led = &g_led_array[led_idx];

    // 检查是否正在运行相同的配置
    if (led->is_running && led->config_state == LED_CFG_APPLIED) {
        if (led->active_config.sequence == config->sequence &&
            led->active_config.schedule.type == config->schedule.type &&
            led->active_config.schedule.param.count == config->schedule.param.count) {
            // 相同配置，不重复应用
            return true;
        }
    }

    // 复制配置到待处理区
    memcpy(&led->pending_config, config, sizeof(LED_RunConfig_t));
    led->config_state = LED_CFG_PENDING;
//	led->config_state = LED_CFG_APPLIED;

    return true;
}

/**
 * @brief 停止LED运行
 */
void LED_Stop(uint8_t led_idx)
{
    if (led_idx >= LED_COUNT) return;
    led_module_stop(&g_led_array[led_idx]);
}

/**
 * @brief 暂停LED
 */
void LED_Pause(uint8_t led_idx)
{
    if (led_idx >= LED_COUNT) return;
    g_led_array[led_idx].pause_requested = true;
    g_led_array[led_idx].enabled = false;
}

/**
 * @brief 恢复LED运行
 */
void LED_Resume(uint8_t led_idx)
{
    if (led_idx >= LED_COUNT) return;
    LED_Object_t *led = &g_led_array[led_idx];
    led->pause_requested = false;
    led->enabled = true;
    led->item_start_time = SysTick_GetTick();
}

/**
 * @brief 获取LED运行状态
 */
bool LED_IsRunning(uint8_t led_idx)
{
    if (led_idx >= LED_COUNT) return false;
    return g_led_array[led_idx].is_running;
}

/**
 * @brief 获取LED使能状态
 */
bool LED_IsEnabled(uint8_t led_idx)
{
    if (led_idx >= LED_COUNT) return false;
    return g_led_array[led_idx].enabled;
}

/**
 * @brief 直接设置LED颜色
 */
void LED_SetColor(uint8_t led_idx, LED_ColorCode_t color)
{
    if (led_idx >= LED_COUNT || color >= LED_COLOR_MAX) return;
    LED_Object_t *led = &g_led_array[led_idx];

    // 停止当前运行
    led_module_stop(led);
    // 直接设置颜色
    led_module_update_hw(led, color);
}

/**
 * @brief 获取当前颜色
 */
LED_ColorCode_t LED_GetCurrentColor(uint8_t led_idx)
{
    if (led_idx >= LED_COUNT) return LED_COLOR_BLACK;
    return g_led_array[led_idx].current_color;
}

/**
 * @brief 获取颜色名称
 */
const char* LED_GetColorName(LED_ColorCode_t color)
{
    if (color >= LED_COLOR_MAX) return "Unknown";
    return g_color_names[color];
}

// ==============================
// 事件设置函数（兼容旧接口）
// ==============================
void led_set_event(uint8_t led_idx, uint32_t evt_bit)
{
    static uint8_t ledx_priority[LED_COUNT_CONFIG] = {0xFF, 0xFF};

    if (led_idx >= LED_COUNT || evt_bit == 0 ) return;

    // 特殊处理：ENABLE/DISABLE/OFF
    if (evt_bit & EVT_LEDx_DISABLE) {
        LED_Stop(led_idx);
        ledx_priority[led_idx] = 0xFF;
        return;
    }

    if (evt_bit & EVT_LEDx_OFF) {
        LED_Stop(led_idx);
        ledx_priority[led_idx] = 0xFF;
        return;
    }
    if (evt_bit & EVT_LEDx_ENABLE) {
        LED_Resume(led_idx);
        return;
    }

        if ( evt_bit >> 28 <= ledx_priority[led_idx] ){

                ledx_priority[led_idx] = (uint8_t)(evt_bit >> 28);

			    evt_bit  &=  0x0FFFFFFF;

                // 转换事件为配置
                LED_EventMapResult_t map_result;
                if (LED_EventToSeqID(evt_bit, &map_result)) {
                                const LED_Sequence_t *seq = LED_GetSequenceByID(map_result.seq_id);
                                if (seq != NULL) {
                                        LED_RunConfig_t config = {
                                                .sequence = seq,
                                                .schedule.type = map_result.sched_type,
                                                .schedule.param = map_result.sched_param,
                                        };
                                LED_ApplyConfig(led_idx, &config);
                        }

                }
        }
}
