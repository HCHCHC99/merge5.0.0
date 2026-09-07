#ifndef __LED_BOOT_H__
#define __LED_BOOT_H__

#include "hc32_ll.h"
#include "Gpio_io.h"

/*=============================================================================
 * Bootloader 升级指示 LED (PC13 / PH2)，低电平点亮
 * 状态机无外部接口，Led_Boot_Task 内部轮询 FlashDownload_GetState() 自动映射
 *============================================================================*/

/* 引脚定义 */
#define LED_BL1_PORT    GPIO_PORT_C         /* PC13 */
#define LED_BL1_PIN     GPIO_PIN_13
#define LED_BL2_PORT    PH2_PORT            /* PH2, 复用 Gpio_io.h 已有宏 */
#define LED_BL2_PIN     PH2_PIN

/* 极性宏：低电平点亮 */
#define LED_ON(port, pin)       GPIO_RESET(port, pin)
#define LED_OFF(port, pin)      GPIO_SET(port, pin)

/* LED 状态枚举（由下载状态自动映射） */
typedef enum
{
    LED_BOOT_IDLE = 0,      /* 双灯 1000ms 闪烁 (1s 亮 1s 灭) */
    LED_BOOT_PROGRAMMING,   /* 双灯 50ms 闪烁 (亮 50ms + 灭 50ms) */
    LED_BOOT_DONE,          /* 双灯 500ms 闪烁 */
    LED_BOOT_FAIL           /* PC13 1000ms 闪烁, PH2 常灭 */
} LedBootState_t;

/* 双灯输出初始化(初始灭), 即进入 1s 慢闪 */
void Led_Boot_Init(void);

/* 非阻塞状态机轮询 (加在 UdsOta_Poll 的 1ms 门控块内) */
void Led_Boot_Task(void);

/* 双灯置灭 + 停用状态机, 跳转 APP 前调用 */
void Led_Boot_Shutdown(void);

#endif /* __LED_BOOT_H__ */
