#ifndef __EVENT_GROUP_H
#define __EVENT_GROUP_H

#include <stdint.h>
#include "sys_config.h"

#if SYS_ENABLE_EVENT_GROUP

#include "event_def.h"
#include "stdbool.h"

// 事件组名称最大长度
#define EVENT_GROUP_NAME_MAX_LEN    16

// 事件位类型（支持32个事件）
typedef uint32_t EventBits_t;

// 单次查询计时状态（用于上层非阻塞循环保存时间戳）
typedef struct {
    uint32_t start_tick;
    bool     is_start;
} EventRecvTimer_t;

// 事件组句柄
typedef struct EventGroup {
    EventBits_t event_bits;
    EventRecvTimer_t timer;
    uint8_t     valid;  // 有效标志
    char        name[EVENT_GROUP_NAME_MAX_LEN]; // 事件组名称
} EventGroup_t;

// 接收模式
#define EVENT_RECV_OR        0x01    // 任意事件触发（或）
#define EVENT_RECV_AND       0x02    // 全部事件触发（与）

/* ============================== 对外接口 ============================== */

/**
 * @brief  创建事件组实例（带名称）
 * @return 事件组句柄
 */
EventGroup_t* EventGroup_Create(const char* name);

/**
 * @brief  发送事件（置位）
 * @param  group: 事件组
 * @param  bits: 事件位
 */
void EventGroup_Send(EventGroup_t* group, EventBits_t bits);

// ------------------------------
// 接收事件（支持 AND / OR 逻辑）
// ------------------------------
EventBits_t EventGroup_Recv(
    EventGroup_t* group,
    EventBits_t wait_bits,
    uint8_t recv_mode,
    uint32_t timeout_ms
);

/**
 * @brief  清除指定事件
 * @param  group: 事件组
 * @param  bits: 要清除的位
 */
void EventGroup_Clear(EventGroup_t* group, EventBits_t bits);

/**
 * @brief  清除所有事件
 * @param  group: 事件组
 */
void EventGroup_ClearAll(EventGroup_t* group);

/**
 * @brief  获取当前所有事件位
 * @param  group: 事件组
 * @return 事件位集合
 */
EventBits_t EventGroup_Get(EventGroup_t* group);

/**
 * @brief  通过 事件名称 获取当前所有事件位
 * @param  group: 事件组
 * @return 事件位集合
 */
EventGroup_t* EventGroup_GetByName(const char* name);

// /**
//  * @brief  检查事件是否置位
//  * @param  group: 事件组
//  * @param  bits: 待检查位
//  * @return 1=存在 0=不存在
//  */
// uint8_t EventGroup_Check(EventGroup_t *group, EventBit_t bits);

#endif

#endif
