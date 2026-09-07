// /**
//  * @file    event_engine.h
//  * @brief   通用事件组引擎（支持多实例、事件位标记、同步机制）
//  * @author  通用架构
//  * @date    2025
//  * @version 1.0
//  * @note    纯事件管理，与状态机引擎松耦合配合
//  */

// #ifndef __EVENT_ENGINE_H
// #define __EVENT_ENGINE_H

// #include <stdint.h>
// #include "sys_config.h"

// #if SYS_ENABLE_EVENT_GROUP

// #ifdef __cplusplus
// extern "C" {
// #endif

// /* ============================== 配置 ============================== */
// #define MAX_EVENT_GROUPS    8   // 最大支持事件组数量

// /* ============================== 类型 ============================== */

// /// 事件位（32 个独立事件）
// typedef uint32_t EventBit_t;

// /// 事件组实例
// typedef struct {
//     EventBit_t bits;      // 事件位集合
//     uint8_t valid;        // 实例有效标志
// } EventGroup_t;

// /* ============================== 对外接口 ============================== */

// /**
//  * @brief  创建事件组实例
//  * @return 事件组句柄
//  */
// EventGroup_t* Event_CreateGroup(void);

// /**
//  * @brief  发送事件（置位）
//  * @param  group: 事件组
//  * @param  bits: 事件位
//  */
// void Event_Send(EventGroup_t *group, EventBit_t bits);

// /**
//  * @brief  清除事件
//  * @param  group: 事件组
//  * @param  bits: 要清除的位
//  */
// void Event_Clear(EventGroup_t *group, EventBit_t bits);

// /**
//  * @brief  清除所有事件
//  * @param  group: 事件组
//  */
// void Event_ClearAll(EventGroup_t *group);

// /**
//  * @brief  获取当前所有事件位
//  * @param  group: 事件组
//  * @return 事件位集合
//  */
// EventBit_t Event_Get(EventGroup_t *group);

// /**
//  * @brief  检查事件是否置位
//  * @param  group: 事件组
//  * @param  bits: 待检查位
//  * @return 1=存在 0=不存在
//  */
// uint8_t Event_Check(EventGroup_t *group, EventBit_t bits);

// #ifdef __cplusplus
// }
// #endif

// #endif /* SYS_ENABLE_EVENT_GROUP */
// #endif /* __EVENT_ENGINE_H */
