/**
 * @file led_event_map.c
 * @brief LED事件映射表实现 - 编译器固化
 */

#include "led_event_map.h"
#include "led_events.h"
#include <string.h>

// ==============================
// 事件到序列ID映射表（编译时确定）
// ==============================
typedef struct {
    uint32_t event_mask;            // 事件掩码
    LED_SeqID_t seq_id;             // 对应的序列ID
    LED_ScheduleType_t sched_type;  // 调度类型
    uint32_t sched_param;           // 调度参数
} EventSeqMap_t;

// 映射表（按优先级从高到低排列）
static const EventSeqMap_t g_event_seq_map[] = {
    // ===== 控制事件（最高优先级） =====
    {EVT_LEDx_DISABLE,          SEQ_ID_SOLID_BLACK, LED_SCHEDULE_INFINITE, 0},
    {EVT_LEDx_OFF,              SEQ_ID_SOLID_BLACK, LED_SCHEDULE_INFINITE, 0},
    
    // ===== 单色常亮 =====
    {EVT_LEDx_RED_ON,           SEQ_ID_SOLID_RED,   LED_SCHEDULE_INFINITE, 0},
    {EVT_LEDx_GREEN_ON,         SEQ_ID_SOLID_GREEN, LED_SCHEDULE_INFINITE, 0},
    {EVT_LEDx_YELLOW_ON,        SEQ_ID_SOLID_YELLOW, LED_SCHEDULE_INFINITE, 0},
    {EVT_LEDx_ALL_ON,           SEQ_ID_SOLID_YELLOW, LED_SCHEDULE_INFINITE, 0},

    // ===== 双色常亮 =====
    {EVT_LEDx_RED_YELLOW_ON,    SEQ_ID_SOLID_RED_YELLOW,   LED_SCHEDULE_INFINITE, 0},
    {EVT_LEDx_RED_GREEN_ON,     SEQ_ID_SOLID_RED_GREEN,    LED_SCHEDULE_INFINITE, 0},
    {EVT_LEDx_GREEN_YELLOW_ON,  SEQ_ID_SOLID_GREEN_YELLOW, LED_SCHEDULE_INFINITE, 0},

//    // ===== 呼吸灯 =====
//    {EVT_LEDx_RED_ON | EVT_LEDx_BLINK_LOOP, 
//                                SEQ_ID_BREATH_RED,  LED_SCHEDULE_INFINITE, 0},
//    {EVT_LEDx_GREEN_ON | EVT_LEDx_BLINK_LOOP, 
//                                SEQ_ID_BREATH_GREEN, LED_SCHEDULE_INFINITE, 0},
//    {EVT_LEDx_YELLOW_ON | EVT_LEDx_BLINK_LOOP, 
//                                SEQ_ID_BREATH_YELLOW, LED_SCHEDULE_INFINITE, 0},
//    
//    // ===== 闪烁 1Hz =====
//    {EVT_LEDx_RED_ON | EVT_LEDx_BLINK_LOOP | EVT_LEDx_FREQ_1HZ_50,
//                                SEQ_ID_BLINK_RED_1HZ, LED_SCHEDULE_INFINITE, 0},
//    {EVT_LEDx_GREEN_ON | EVT_LEDx_BLINK_LOOP | EVT_LEDx_FREQ_1HZ_50,
//                                SEQ_ID_BLINK_GREEN_1HZ, LED_SCHEDULE_INFINITE, 0},
//    {EVT_LEDx_YELLOW_ON | EVT_LEDx_BLINK_LOOP | EVT_LEDx_FREQ_1HZ_50,
//                                SEQ_ID_BLINK_YELLOW_1HZ, LED_SCHEDULE_INFINITE, 0},
//    
    // ===== 闪烁 2Hz =====
    {EVT_LEDx_RED_ON | EVT_LEDx_BLINK_LOOP | EVT_LEDx_FREQ_2HZ_50,
                                SEQ_ID_BLINK_RED_2HZ, LED_SCHEDULE_INFINITE, 0},
    {EVT_LEDx_GREEN_ON | EVT_LEDx_BLINK_LOOP | EVT_LEDx_FREQ_2HZ_50,
                                SEQ_ID_BLINK_GREEN_2HZ, LED_SCHEDULE_INFINITE, 0},
    {EVT_LEDx_YELLOW_ON | EVT_LEDx_BLINK_LOOP | EVT_LEDx_FREQ_2HZ_50,
                                SEQ_ID_BLINK_YELLOW_2HZ, LED_SCHEDULE_INFINITE, 0},
    
//    // ===== 闪烁 5Hz =====
//    {EVT_LEDx_RED_ON | EVT_LEDx_BLINK_LOOP | EVT_LEDx_FREQ_5HZ_50,
//                                SEQ_ID_BLINK_RED_5HZ, LED_SCHEDULE_INFINITE, 0},
//    {EVT_LEDx_GREEN_ON | EVT_LEDx_BLINK_LOOP | EVT_LEDx_FREQ_5HZ_50,
//                                SEQ_ID_BLINK_GREEN_5HZ, LED_SCHEDULE_INFINITE, 0},
//    {EVT_LEDx_YELLOW_ON | EVT_LEDx_BLINK_LOOP | EVT_LEDx_FREQ_5HZ_50,
//                                SEQ_ID_BLINK_YELLOW_5HZ, LED_SCHEDULE_INFINITE, 0},
	
    // ===== 闪烁 10Hz =====
    {EVT_LEDx_RED_ON | EVT_LEDx_BLINK_LOOP | EVT_LEDx_FREQ_10HZ_50,
                                SEQ_ID_BLINK_RED_10HZ, LED_SCHEDULE_INFINITE, 0},
    {EVT_LEDx_GREEN_ON | EVT_LEDx_BLINK_LOOP | EVT_LEDx_FREQ_10HZ_50,
                                SEQ_ID_BLINK_GREEN_10HZ, LED_SCHEDULE_INFINITE, 0},
    {EVT_LEDx_YELLOW_ON | EVT_LEDx_BLINK_LOOP | EVT_LEDx_FREQ_10HZ_50,
                                SEQ_ID_BLINK_YELLOW_10HZ, LED_SCHEDULE_INFINITE, 0},		
    {EVT_LEDx_RED_YELLOW_ON | EVT_LEDx_BLINK_LOOP | EVT_LEDx_FREQ_10HZ_50,
                                SEQ_ID_BLINK_RED_YELLOW_10HZ, LED_SCHEDULE_INFINITE, 0},	
	
    // ===== 自定义闪烁 2xHz =====
    {EVT_LEDx_RED_ON | EVT_LEDx_BLINK_LOOP | EVT_LEDx_FREQ_2xHZ_50,
                                SEQ_ID_BLINK_RED_2xHZ, LED_SCHEDULE_INFINITE, 0},
    {EVT_LEDx_GREEN_ON | EVT_LEDx_BLINK_LOOP | EVT_LEDx_FREQ_2xHZ_50,
                                SEQ_ID_BLINK_GREEN_2xHZ, LED_SCHEDULE_INFINITE, 0},
    {EVT_LEDx_YELLOW_ON | EVT_LEDx_BLINK_LOOP | EVT_LEDx_FREQ_2xHZ_50,
                                SEQ_ID_BLINK_YELLOW_2xHZ, LED_SCHEDULE_INFINITE, 0},						

    // ===== 自定义闪烁 3xHz =====
    {EVT_LEDx_RED_ON | EVT_LEDx_BLINK_LOOP | EVT_LEDx_FREQ_3xHZ_50,
                                SEQ_ID_BLINK_RED_3xHZ, LED_SCHEDULE_INFINITE, 0},
    {EVT_LEDx_GREEN_ON | EVT_LEDx_BLINK_LOOP | EVT_LEDx_FREQ_3xHZ_50,
                                SEQ_ID_BLINK_GREEN_3xHZ, LED_SCHEDULE_INFINITE, 0},
    {EVT_LEDx_YELLOW_ON | EVT_LEDx_BLINK_LOOP | EVT_LEDx_FREQ_3xHZ_50,
                                SEQ_ID_BLINK_YELLOW_3xHZ, LED_SCHEDULE_INFINITE, 0},									
								
	
//    // ===== 闪烁带次数（默认2Hz） =====
//    {EVT_LEDx_RED_ON | EVT_LEDx_BLINK_CNT_3,
//                                SEQ_ID_BLINK_RED_2HZ, LED_SCHEDULE_COUNT, 3},
//    {EVT_LEDx_RED_ON | EVT_LEDx_BLINK_CNT_5,
//                                SEQ_ID_BLINK_RED_2HZ, LED_SCHEDULE_COUNT, 5},
//    {EVT_LEDx_GREEN_ON | EVT_LEDx_BLINK_CNT_3,
//                                SEQ_ID_BLINK_GREEN_2HZ, LED_SCHEDULE_COUNT, 3},
//    {EVT_LEDx_GREEN_ON | EVT_LEDx_BLINK_CNT_5,
//                                SEQ_ID_BLINK_GREEN_2HZ, LED_SCHEDULE_COUNT, 5},
//    {EVT_LEDx_YELLOW_ON | EVT_LEDx_BLINK_CNT_3,
//                                SEQ_ID_BLINK_YELLOW_2HZ, LED_SCHEDULE_COUNT, 3},
//    {EVT_LEDx_YELLOW_ON | EVT_LEDx_BLINK_CNT_5,
//                                SEQ_ID_BLINK_YELLOW_2HZ, LED_SCHEDULE_COUNT, 5},
//    
//    // ===== 闪烁带时间（默认2Hz） =====
//    {EVT_LEDx_RED_ON | EVT_LEDx_BLINK_TIME_2S,
//                                SEQ_ID_BLINK_RED_2HZ, LED_SCHEDULE_TIME, 2000},
//    {EVT_LEDx_RED_ON | EVT_LEDx_BLINK_TIME_5S,
//                                SEQ_ID_BLINK_RED_2HZ, LED_SCHEDULE_TIME, 5000},
//    {EVT_LEDx_GREEN_ON | EVT_LEDx_BLINK_TIME_2S,
//                                SEQ_ID_BLINK_GREEN_2HZ, LED_SCHEDULE_TIME, 2000},
//    {EVT_LEDx_GREEN_ON | EVT_LEDx_BLINK_TIME_5S,
//                                SEQ_ID_BLINK_GREEN_2HZ, LED_SCHEDULE_TIME, 5000},
//    
//    // ===== 双闪 =====
//    {EVT_LEDx_RED_ON | EVT_LEDx_BLINK_INTERVAL_2,
//                                SEQ_ID_DOUBLE_BLINK_RED, LED_SCHEDULE_INFINITE, 0},
//    {EVT_LEDx_GREEN_ON | EVT_LEDx_BLINK_INTERVAL_2,
//                                SEQ_ID_DOUBLE_BLINK_GREEN, LED_SCHEDULE_INFINITE, 0},
//    {EVT_LEDx_YELLOW_ON | EVT_LEDx_BLINK_INTERVAL_2,
//                                SEQ_ID_DOUBLE_BLINK_YELLOW, LED_SCHEDULE_INFINITE, 0},
//    
//    // ===== 间隔模式 =====
//    {EVT_LEDx_RED_ON | EVT_LEDx_BLINK_INTERVAL_2,
//                                SEQ_ID_INTERVAL_2_RED, LED_SCHEDULE_INFINITE, 0},
//    {EVT_LEDx_GREEN_ON | EVT_LEDx_BLINK_INTERVAL_2,
//                                SEQ_ID_INTERVAL_2_GREEN, LED_SCHEDULE_INFINITE, 0},
//    {EVT_LEDx_RED_ON | EVT_LEDx_BLINK_INTERVAL_3,
//                                SEQ_ID_INTERVAL_3_RED, LED_SCHEDULE_INFINITE, 0},
//    {EVT_LEDx_GREEN_ON | EVT_LEDx_BLINK_INTERVAL_3,
//                                SEQ_ID_INTERVAL_3_GREEN, LED_SCHEDULE_INFINITE, 0},
    
    // ===== 交替闪烁 =====
    {EVT_LEDx_RED_GREEN_ON | EVT_LEDx_BLINK_ALT,
                                SEQ_ID_ALT_BLINK_RED_GREEN, LED_SCHEDULE_INFINITE, 0},
    {EVT_LEDx_RED_YELLOW_ON | EVT_LEDx_BLINK_ALT,
                                SEQ_ID_ALT_BLINK_RED_YELLOW, LED_SCHEDULE_INFINITE, 0},
    {EVT_LEDx_GREEN_YELLOW_ON | EVT_LEDx_BLINK_ALT,
                                SEQ_ID_ALT_BLINK_GREEN_YELLOW, LED_SCHEDULE_INFINITE, 0},
    
//    // ===== 特殊模式 =====
//    {EVT_LEDx_ALL_ON | EVT_LEDx_BLINK_LOOP,
//                                SEQ_ID_RGB_FLOW, LED_SCHEDULE_INFINITE, 0},
//    {EVT_LEDx_ALL_ON | EVT_LEDx_BLINK_ALT,
//                                SEQ_ID_POLICE, LED_SCHEDULE_INFINITE, 0},
//    {EVT_LEDx_RED_ON | EVT_LEDx_BLINK_ALT | EVT_LEDx_BLINK_LOOP,
//                                SEQ_ID_HEARTBEAT, LED_SCHEDULE_INFINITE, 0},
};

// ==============================
// 降级处理函数（当精确匹配失败时）
// ==============================
static bool event_to_config_fallback(uint32_t evt_bit, LED_EventMapResult_t *result)
{
    // 提取颜色
    LED_ColorCode_t color = LED_COLOR_BLACK;
    LED_SeqID_t base_seq = SEQ_ID_SOLID_BLACK;
    
    if (evt_bit & EVT_LEDx_RED_ON) {
        color = LED_COLOR_RED;
        base_seq = SEQ_ID_BLINK_RED_10HZ;
    } else if (evt_bit & EVT_LEDx_GREEN_ON) {
        color = LED_COLOR_GREEN;
        base_seq = SEQ_ID_BLINK_GREEN_10HZ;
    } else if (evt_bit & EVT_LEDx_YELLOW_ON) {
        color = LED_COLOR_YELLOW;
        base_seq = SEQ_ID_BLINK_YELLOW_10HZ;
    } else if (evt_bit & EVT_LEDx_RED_YELLOW_ON) {
        color = LED_COLOR_RED_YELLOW;
        base_seq = SEQ_ID_BLINK_RED_YELLOW_10HZ;
    } else if (evt_bit & EVT_LEDx_ALL_ON) {
        color = LED_COLOR_YELLOW;
        base_seq = SEQ_ID_BLINK_YELLOW_10HZ;
    }
    
    // 如果没有颜色，默认红色
    if (color == LED_COLOR_BLACK && (evt_bit & EVT_LEDx_BLINK_LOOP)) {
        color = LED_COLOR_RED;
        base_seq = SEQ_ID_BLINK_RED_10HZ;
    }
    
    // 如果完全没有有效事件，返回失败
    if (color == LED_COLOR_BLACK && !(evt_bit & EVT_LEDx_BLINK_LOOP)) {
        return false;
    }
    
    // 检查频率
    LED_SeqID_t freq_seq = base_seq;
    if (evt_bit & EVT_LEDx_FREQ_1HZ_50) {
//        // 根据颜色选择对应的1Hz序列
//        if (color == LED_COLOR_RED) freq_seq = SEQ_ID_BLINK_RED_1HZ;
//        else if (color == LED_COLOR_GREEN) freq_seq = SEQ_ID_BLINK_GREEN_1HZ;
//        else if (color == LED_COLOR_YELLOW) freq_seq = SEQ_ID_BLINK_YELLOW_1HZ;
    } else if (evt_bit & EVT_LEDx_FREQ_2HZ_50) {
        if (color == LED_COLOR_RED) freq_seq = SEQ_ID_BLINK_RED_2HZ;
        else if (color == LED_COLOR_GREEN) freq_seq = SEQ_ID_BLINK_GREEN_2HZ;
        else if (color == LED_COLOR_YELLOW) freq_seq = SEQ_ID_BLINK_YELLOW_2HZ;
    } else if (evt_bit & EVT_LEDx_FREQ_5HZ_50) {
//        if (color == LED_COLOR_RED) freq_seq = SEQ_ID_BLINK_RED_5HZ;
//        else if (color == LED_COLOR_GREEN) freq_seq = SEQ_ID_BLINK_GREEN_5HZ;
//        else if (color == LED_COLOR_YELLOW) freq_seq = SEQ_ID_BLINK_YELLOW_5HZ;
    } else if (evt_bit & EVT_LEDx_FREQ_10HZ_50) {
        if (color == LED_COLOR_RED) freq_seq = SEQ_ID_BLINK_RED_10HZ;
        else if (color == LED_COLOR_GREEN) freq_seq = SEQ_ID_BLINK_GREEN_10HZ;
        else if (color == LED_COLOR_YELLOW) freq_seq = SEQ_ID_BLINK_YELLOW_10HZ;
		else if (color == LED_COLOR_RED_YELLOW) freq_seq = SEQ_ID_BLINK_RED_YELLOW_10HZ;
    } else if (evt_bit & EVT_LEDx_FREQ_2xHZ_50){
		if (color == LED_COLOR_RED) freq_seq = SEQ_ID_BLINK_RED_2xHZ;
		else if (color == LED_COLOR_GREEN) freq_seq = SEQ_ID_BLINK_GREEN_2xHZ;
		else if (color == LED_COLOR_YELLOW) freq_seq = SEQ_ID_BLINK_YELLOW_2xHZ;
    } else if (evt_bit & EVT_LEDx_FREQ_3xHZ_50){
		if (color == LED_COLOR_RED) freq_seq = SEQ_ID_BLINK_RED_3xHZ;
		else if (color == LED_COLOR_GREEN) freq_seq = SEQ_ID_BLINK_GREEN_3xHZ;
		else if (color == LED_COLOR_YELLOW) freq_seq = SEQ_ID_BLINK_YELLOW_3xHZ;
	}
    
    // 输出结果
    result->seq_id = freq_seq;
    result->sched_type = LED_SCHEDULE_INFINITE;
    result->sched_param = 0;
    result->valid = true;
    
    return true;
}

// ==============================
// 公共接口实现
// ==============================
bool LED_EventToSeqID(uint32_t evt_bit, LED_EventMapResult_t *result)
{
    if (result == NULL) return false;

	if (evt_bit == 0) {
		return false;
	}
		
    // 清空结果
    memset(result, 0, sizeof(LED_EventMapResult_t));
    
    // 1. 先尝试精确匹配（按优先级顺序）
    for (uint8_t i = 0; i < sizeof(g_event_seq_map) / sizeof(EventSeqMap_t); i++) {
        // 检查事件是否完全匹配
        if ((evt_bit & g_event_seq_map[i].event_mask) == evt_bit) {
            result->seq_id = g_event_seq_map[i].seq_id;
            result->sched_type = g_event_seq_map[i].sched_type;
            result->sched_param = g_event_seq_map[i].sched_param;
            result->valid = true;
            return true;
        }
    }
    
    // 2. 尝试降级匹配
    if (event_to_config_fallback(evt_bit, result)) {
        return true;
    }
    
    // 3. 完全无匹配
    return false;
}

