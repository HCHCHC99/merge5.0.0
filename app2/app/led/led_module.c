// #include "sys_sched.h"
// #include "led.h"

// static const SysModule_t LED_Module = {
//     .name = "LED",
//     .prio = 4,
//     .period_ms = 1,
//     .enabled = 1,
//     .init = LED_Init,
//     .task = LED_Task,
// };

// void LED_Module_Register(void) {
//     Sys_Scheduler_RegisterModule(&LED_Module);
// }

/**
 * @file led_module.c
 * @brief LED核心模块实现
 */

#include "led_module.h"
#include "led_sequence.h"
#include "led_seq_id.h"
#include "sys_tick.h"
#include "hc32_ll_gpio.h"
#include "hc32_ll_utility.h"
#include <string.h>
#include <stdlib.h>

// ==============================
// 颜色配置表
// ==============================
static const LED_ColorMap_t g_color_table[] = {
    [LED_COLOR_BLACK]         = {0, 0, 0},
    [LED_COLOR_RED]           = {1, 0, 0},
    [LED_COLOR_GREEN]         = {0, 1, 0},
    [LED_COLOR_YELLOW]        = {0, 0, 1},
	[LED_COLOR_RED_YELLOW]    = {1, 0, 1},
	[LED_COLOR_RED_GREEN]     = {1, 1, 0},
	[LED_COLOR_GREEN_YELLOW]  = {0, 1, 1},
};

// ==============================
// LED对象数组（内部静态）
// ==============================
LED_Object_t g_led_array[LED_COUNT_CONFIG];
const uint8_t LED_COUNT = LED_COUNT_CONFIG;

// ==============================
// 静态函数
// ==============================
static inline void led_set_pin(uint8_t gpio_port, uint16_t pin, uint8_t level);

/**
 * @brief GPIO引脚设置
 */
static inline void led_set_pin(uint8_t gpio_port, uint16_t pin, uint8_t level)
{
    if (level) {
        GPIO_SetPins(gpio_port, pin);
    } else {
        GPIO_ResetPins(gpio_port, pin);
    }
}

/**
 * @brief 更新硬件输出
 */
void led_module_update_hw(LED_Object_t *led, LED_ColorCode_t color)
{
    if (!led || color >= LED_COLOR_MAX) return;

    const LED_ColorMap_t *map = &g_color_table[color];
    uint8_t act = led->hw.active_level;

    // 红色
    led_set_pin(led->hw.gpio_r, led->hw.pin_r, map->r_enable ? act : !act);
    // 绿色
    led_set_pin(led->hw.gpio_g, led->hw.pin_g, map->g_enable ? act : !act);
    // 黄色
    led_set_pin(led->hw.gpio_y, led->hw.pin_y, map->y_enable ? act : !act);

    led->current_color = color;
}

/**
 * @brief 应用当前序列项
 */
static void led_apply_current_item(LED_Object_t *led)
{
    if (!led || !led->active_config.sequence) return;

    const LED_SequenceItem_t *item = &led->active_config.sequence->items[led->current_item_index];
    led_module_update_hw(led, item->color);
    led->item_start_time = SysTick_GetTick();
}

/**
 * @brief 处理序列结束
 */
static bool led_handle_sequence_end(LED_Object_t *led)
{
    if (!led || !led->active_config.sequence) return false;

    // 更新调度统计
    led->schedule_current_count++;
    led->schedule_elapsed_time_ms += led->active_config.sequence->total_duration_ms;

    const LED_ScheduleParam_t *sched = &led->active_config.schedule;

    switch (sched->type) {
        case LED_SCHEDULE_INFINITE:
            led->current_item_index = 0;
            led_apply_current_item(led);
            return true;

        case LED_SCHEDULE_COUNT:
            if (led->schedule_current_count >= sched->param.count) {
                led_module_stop(led);
                return false;
            }
            led->current_item_index = 0;
            led_apply_current_item(led);
            return true;

        case LED_SCHEDULE_TIME:
            if (led->schedule_elapsed_time_ms >= sched->param.time_ms) {
                led_module_stop(led);
                return false;
            }
            led->current_item_index = 0;
            led_apply_current_item(led);
            return true;

        default:
            led_module_stop(led);
            return false;
    }
}

/**
 * @brief 切换到下一个序列项
 */
bool led_module_next_item(LED_Object_t *led)
{
    if (!led || !led->active_config.sequence) return false;

    led->current_item_index++;

    if (led->current_item_index >= led->active_config.sequence->item_count) {
        return led_handle_sequence_end(led);
    }

    led_apply_current_item(led);
    return true;
}

/**
 * @brief 启动运行
 */
void led_module_start(LED_Object_t *led)
{
    if (!led) return;

    led->is_running = true;
    led->enabled = true;
    led->current_item_index = 0;
    led->schedule_current_count = 0;
    led->schedule_elapsed_time_ms = 0;
    led->pause_requested = false;
    led->config_state = LED_CFG_APPLIED;

    if (led->active_config.sequence && led->active_config.sequence->item_count > 0) {
        led_apply_current_item(led);
    } else {
        // 无效序列，关闭LED
        led_module_update_hw(led, LED_COLOR_BLACK);
        led->is_running = false;
    }
}

/**
 * @brief 停止运行
 */
void led_module_stop(LED_Object_t *led)
{
    if (!led) return;

    led->is_running = false;
    led->enabled = false;
    led->config_state = LED_CFG_IDLE;
    led_module_update_hw(led, LED_COLOR_BLACK);
}

/**
 * @brief 应用待处理配置
 */
bool led_module_apply_pending(LED_Object_t *led)
{
    if (!led || led->config_state != LED_CFG_PENDING) return false;

    // 复制待处理配置到活动配置
    memcpy(&led->active_config, &led->pending_config, sizeof(LED_RunConfig_t));

    // 清空待处理配置
    memset(&led->pending_config, 0, sizeof(LED_RunConfig_t));
    led->config_state = LED_CFG_APPLYING;

    // 启动新配置
    led_module_start(led);

    return true;
}

/**
 * @brief GPIO初始化
 */
void led_hw_init(LED_Object_t *led)
{
    stc_gpio_init_t stcGpioInit;
    GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PinDir = PIN_DIR_OUT;
    stcGpioInit.u16PullUp = led->hw.active_level ? PIN_STAT_RST : PIN_STAT_SET;

    GPIO_Init(led->hw.gpio_r, led->hw.pin_r, &stcGpioInit);
    GPIO_Init(led->hw.gpio_g, led->hw.pin_g, &stcGpioInit);
    GPIO_Init(led->hw.gpio_y, led->hw.pin_y, &stcGpioInit);
}
