#ifndef __KEY_H
#define __KEY_H

#include <stdint.h>
#include <stdbool.h>
#include "state_engine.h"
#include "event_def.h"
#include "sys_tick.h"

#include "hc32f460.h"
#include "hc32_ll.h"
#include "hc32_ll_gpio.h"

/*****************************************
 * 按键配置宏
 ****************************************/
#define KEY_MAX_COUNT        6    /* 最大支持按键数量 */
#define KEY_SCAN_BUFFER_LEN  10   /* 按键消抖深度 */

/*****************************************
 * 按键事件类型（统一标准：所有键共用）
 ****************************************/
typedef enum {
    KEY_EVENT_NONE    = 0U,   /* 无事件 */
    KEY_EVENT_CLICK   = 1U,   /* 单击事件 */
    KEY_EVENT_LONG    = 2U,   /* 长按事件 */
    KEY_EVENT_RELEASE = 3U,   /* 释放事件 */
} Key_Event_t;

/*****************************************
 * 键盘对外统一上报结构体（核心出口）
 * 物理按键 / 组合按键 完全一致
 ****************************************/
typedef struct {
    uint32_t    key_code;      /* 唯一按键码（物理键/虚拟组合键） */
    const char* key_name;      /* 按键名称 */
    Key_Event_t event;         /* 按键事件 */
    uint32_t    timestamp;     /* 事件触发时间戳 */
} Key_Report_t;

/*****************************************
 * 按键硬件配置结构体
 ****************************************/
typedef struct {
    uint8_t  gpio;             /* GPIO端口号（自定义编号） */
    uint32_t pin;              /* GPIO引脚 */
    uint8_t  active_level;     /* 有效电平 0-低电平 1-高电平 */
    uint16_t long_press_ms;    /* 长按判定阈值(ms) */
} Key_HwConfig_t;

/*****************************************
 * 组合按键映射表（虚拟成标准按键）
 ****************************************/
typedef struct {
    uint8_t  key_index[8];     /* 参与组合的物理按键编号 */
    uint8_t  key_num;          /* 组合按键数量 */
    uint32_t long_ms;          /* 长按触发时间 */
    uint32_t virtual_code;     /* 虚拟按键码 */
    const char* virtual_name;  /* 虚拟按键名称 */
} Key_Combine_Map_t;

/*****************************************
 * 按键对象（每个按键独立属性）
 ****************************************/
typedef struct {
    uint8_t              key_id;        /* 按键ID */
    const Key_HwConfig_t *hw;           /* 硬件配置 */
    StateMachine_t       sm;            /* 状态机 */

    uint8_t              stable_now;    /* 当前稳定电平 */
    uint8_t              stable_last;   /* 上一次稳定电平 */

    uint8_t              locked;        /* 组合键锁定标志 */

    uint32_t             tick_press;    /* 按下时间戳 */
    uint32_t             tick_release;  /* 释放时间戳 */
    uint32_t             long_timer;     /* 长按计时器 */

    // /* 事件组*/   // 不再通过事件组完成，而是通过发布订阅机制，传递按键值、事件组和时间戳
    // EventGroup_t *evt_key;

} Key_Object_t;

/*****************************************
 * 对外接口
 ****************************************/
void Key_Init(void);
void Key_Task(void);

#endif /* __KEY_H */
