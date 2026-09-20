#ifndef __LED_BOOT_H__
#define __LED_BOOT_H__

#include "hc32_ll.h"
#include "Gpio_io.h"

/*=============================================================================
 * Bootloader 升级指示 LED (PC13 / PH2)，低电平点亮
 * 状态机无外部接口, Led_Boot_Task 内部轮询 FlashDownload_GetState() 自动映射:
 *   IDLE/PREPARING        -> 灭 (刚进 bootloader, 未刷写)
 *   READY/TRANSFERRING... -> 50ms 快闪 (刷写中)
 *   COMPLETE              -> 常亮 (刷写成功)
 *   ERROR                 -> 灭 (刷写失败)
 *   STAY(工装停留)        -> 常亮 (锁存, 仅跳转前 Shutdown 熄灭)
 *============================================================================*/

/* 引脚定义 */
#define LED_BL1_PORT    GPIO_PORT_C         /* PC13 */
#define LED_BL1_PIN     GPIO_PIN_13
#define LED_BL2_PORT    PH2_PORT            /* PH2, 复用 Gpio_io.h 已有宏 */
#define LED_BL2_PIN     PH2_PIN

/* 极性宏：低电平点亮 */
#define LED_ON(port, pin)       GPIO_RESET(port, pin)
#define LED_OFF(port, pin)      GPIO_SET(port, pin)

/* LED 状态枚举 */
typedef enum
{
    LED_BOOT_IDLE = 0,      /* 未刷写: 双灯灭 */
    LED_BOOT_PROGRAMMING,   /* 刷写中: 双灯 50ms 闪烁 */
    LED_BOOT_DONE,          /* 刷写完成: 双灯 500ms 闪烁 */
    LED_BOOT_FAIL,          /* 刷写失败: 双灯灭 */
    LED_BOOT_STAY           /* 工装升级成功停留: 双灯常亮 (锁存) */
} LedBootState_t;

/* 双灯输出初始化(灭), 即进入 IDLE 灭灯态 */
void Led_Boot_Init(void);

/* 非阻塞状态机轮询 (加在 UdsOta_Poll 的 1ms 门控块内) */
void Led_Boot_Task(void);

/* 进入工装停留态: 双灯常亮 (锁存, 不再随下载状态自动切换) */
void Led_Boot_Stay(void);

/* 双灯置灭 + 停用状态机, 跳转 APP 前调用 */
void Led_Boot_Shutdown(void);

#endif /* __LED_BOOT_H__ */
