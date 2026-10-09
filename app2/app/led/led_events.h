/**
 * @file led_events.h
 * @brief LED事件定义 - 供其他模块发送事件使用
 */

#ifndef __LED_EVENTS_H__
#define __LED_EVENTS_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// ==============================
// LED事件位定义
// ==============================
// 控制事件
#define EVT_LEDx_ENABLE             (1UL << 0)      // 0x0000 0001
#define EVT_LEDx_DISABLE            (1UL << 1)      // 0x0000 0002
#define EVT_LEDx_OFF                (1UL << 2)      // 0x0000 0004

// 颜色事件
#define EVT_LEDx_RED_ON             (1UL << 3)      // 0x0000 0008
#define EVT_LEDx_GREEN_ON           (1UL << 4)      // 0x0000 0010
#define EVT_LEDx_YELLOW_ON          (1UL << 5)      // 0x0000 0020
#define EVT_LEDx_RED_YELLOW_ON      (1UL << 6)      // 0x0000 0040
#define EVT_LEDx_RED_GREEN_ON       (1UL << 7)      // 0x0000 0080 (与GREEN_ON冲突，保留兼容)
#define EVT_LEDx_GREEN_YELLOW_ON    (1UL << 8)      // 0x0000 0100
#define EVT_LEDx_ALL_ON             (1UL << 9)      // 0x0000 0200

// 闪烁模式事件
#define EVT_LEDx_BLINK_LOOP         (1UL << 10)     // 0x0000 0400
#define EVT_LEDx_BLINK_CNT_3        (1UL << 11)     // 0x0000 0800
#define EVT_LEDx_BLINK_CNT_5        (1UL << 12)     // 0x0000 1000
#define EVT_LEDx_BLINK_TIME_2S      (1UL << 13)     // 0x0000 2000
#define EVT_LEDx_BLINK_TIME_5S      (1UL << 14)     // 0x0000 4000
#define EVT_LEDx_BLINK_ALT          (1UL << 15)     // 0x0000 8000
#define EVT_LEDx_BLINK_INTERVAL_2   (1UL << 16)     // 0x0001 0000
#define EVT_LEDx_BLINK_INTERVAL_3   (1UL << 17)     // 0x0002 0000

// 频率事件（兼容旧版）
#define EVT_LEDx_FREQ1              (1UL << 18)     // 0x0004 0000
#define EVT_LEDx_FREQ2              (1UL << 19)     // 0x0008 0000
#define EVT_LEDx_FREQ3              (1UL << 20)     // 0x0010 0000
#define EVT_LEDx_FREQ4              (1UL << 21)     // 0x0020 0000

// 频率事件（新版）
#define EVT_LEDx_FREQ_1HZ_50        (EVT_LEDx_FREQ1) // 500ms+ 500ms
#define EVT_LEDx_FREQ_2HZ_50        (EVT_LEDx_FREQ2) // 250ms+ 250ms
#define EVT_LEDx_FREQ_5HZ_50        (EVT_LEDx_FREQ3) // 100ms+ 100ms
#define EVT_LEDx_FREQ_10HZ_50       (EVT_LEDx_FREQ4) // 50ms+ 50ms
#define EVT_LEDx_FREQ_2xHZ_50       (1UL << 22)     // 0xa0040 0000
#define EVT_LEDx_FREQ_3xHZ_50       (1UL << 23)     // 0x0080 0000

//单色闪烁指令：颜色|EVT_LEDx_BLINK_LOOP|频率 | 优先级
//双色闪烁指令：颜色|EVT_LEDx_BLINK_ALT|频率 | 优先级

// 指示灯优先级
#define EVT_LEDx_PRIORITY_1         (1UL << 28)      // 0x1000 0000
#define EVT_LEDx_PRIORITY_2         (2UL << 28)      // 0x2000 0000
#define EVT_LEDx_PRIORITY_3         (3UL << 28)      // 0x3000 0000
#define EVT_LEDx_PRIORITY_4         (4UL << 28)      // 0x4000 0000
#define EVT_LEDx_PRIORITY_5         (5UL << 28)      // 0x5000 0000
#define EVT_LEDx_PRIORITY_6         (6UL << 28)      // 0x6000 0000
#define EVT_LEDx_PRIORITY_7         (7UL << 28)      // 0x7000 0000
#define EVT_LEDx_PRIORITY_8         (8UL << 28)      // 0x8000 0000

#ifdef __cplusplus
}
#endif

#endif /* __LED_EVENTS_H__ */
