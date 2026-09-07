/**
 * @file led_seq_id.h
 * @brief LED序列ID定义 - 编译时固化
 */

#ifndef __LED_SEQ_ID_H__
#define __LED_SEQ_ID_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ==============================
// LED序列ID（编译时固化，存储在ROM）
// ==============================
typedef enum {
    // -------- 单色常亮序列 --------
    SEQ_ID_SOLID_RED,           // 红色常亮
    SEQ_ID_SOLID_GREEN,         // 绿色常亮
    SEQ_ID_SOLID_YELLOW,        // 黄色常亮
    SEQ_ID_SOLID_BLACK,         // 全灭

    // -------- 双色常亮序列 --------
    SEQ_ID_SOLID_RED_YELLOW,
    SEQ_ID_SOLID_RED_GREEN,
    SEQ_ID_SOLID_GREEN_YELLOW,

//   // -------- 呼吸灯序列 --------
//   SEQ_ID_BREATH_RED,
//   SEQ_ID_BREATH_GREEN,
//   SEQ_ID_BREATH_YELLOW,

//    // -------- 标准闪烁序列（1Hz） --------
//    SEQ_ID_BLINK_RED_1HZ,
//    SEQ_ID_BLINK_GREEN_1HZ,
//    SEQ_ID_BLINK_YELLOW_1HZ,
//
    // -------- 标准闪烁序列（2Hz） --------
    SEQ_ID_BLINK_RED_2HZ,
    SEQ_ID_BLINK_GREEN_2HZ,
    SEQ_ID_BLINK_YELLOW_2HZ,
//
//    // -------- 标准闪烁序列（5Hz） --------
//    SEQ_ID_BLINK_RED_5HZ,
//    SEQ_ID_BLINK_GREEN_5HZ,
//    SEQ_ID_BLINK_YELLOW_5HZ,

    // -------- 标准闪烁序列（10Hz） --------
    SEQ_ID_BLINK_RED_10HZ,
    SEQ_ID_BLINK_GREEN_10HZ,
    SEQ_ID_BLINK_YELLOW_10HZ,
	SEQ_ID_BLINK_RED_YELLOW_10HZ,

    // -------- 自定义闪烁序列（2xHz） --------
    SEQ_ID_BLINK_RED_2xHZ,
    SEQ_ID_BLINK_GREEN_2xHZ,
    SEQ_ID_BLINK_YELLOW_2xHZ,

    // -------- 自定义闪烁序列（3xHz） --------
    SEQ_ID_BLINK_RED_3xHZ,
    SEQ_ID_BLINK_GREEN_3xHZ,
    SEQ_ID_BLINK_YELLOW_3xHZ,

//    // -------- 双闪序列 --------
//    SEQ_ID_DOUBLE_BLINK_RED,
//    SEQ_ID_DOUBLE_BLINK_GREEN,
//    SEQ_ID_DOUBLE_BLINK_YELLOW,

    // -------- 交替闪烁序列 --------
    SEQ_ID_ALT_BLINK_RED_GREEN,
    SEQ_ID_ALT_BLINK_RED_YELLOW,
    SEQ_ID_ALT_BLINK_GREEN_YELLOW,

//    // -------- 特殊模式序列 --------
//    SEQ_ID_HEARTBEAT,
//    SEQ_ID_POLICE,
//    SEQ_ID_SOS,
//    SEQ_ID_RGB_FLOW,
//
//    // -------- 间隔模式序列 --------
//    SEQ_ID_INTERVAL_2_RED,
//    SEQ_ID_INTERVAL_2_GREEN,
//    SEQ_ID_INTERVAL_3_RED,
//    SEQ_ID_INTERVAL_3_GREEN,

    SEQ_ID_COUNT,               // 序列总数（编译器自动计算）
} LED_SeqID_t;

#ifdef __cplusplus
}
#endif

#endif /* __LED_SEQ_ID_H__ */
