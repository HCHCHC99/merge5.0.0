/**
 * @file led_event_map.h
 * @brief LED事件映射表 - 编译器固化
 */

#ifndef __LED_EVENT_MAP_H__
#define __LED_EVENT_MAP_H__

#include <stdint.h>
#include "led.h"
#include "led_seq_id.h"

#ifdef __cplusplus
extern "C" {
#endif

// ==============================
// 事件映射结果
// ==============================
typedef struct {
    LED_SeqID_t seq_id;             // 序列ID
    LED_ScheduleType_t sched_type;  // 调度类型
    uint32_t sched_param;           // 调度参数
    bool valid;                     // 是否有效
} LED_EventMapResult_t;

// ==============================
// 事件映射函数
// ==============================

/**
 * @brief 将事件位转换为序列ID和调度参数
 * @param evt_bit 事件位
 * @param result 输出结果
 * @return true:成功映射 false:无匹配
 */
bool LED_EventToSeqID(uint32_t evt_bit, LED_EventMapResult_t *result);

#ifdef __cplusplus
}
#endif

#endif /* __LED_EVENT_MAP_H__ */
