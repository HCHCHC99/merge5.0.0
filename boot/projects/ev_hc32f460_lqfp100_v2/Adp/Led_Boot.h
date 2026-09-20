#ifndef __LED_BOOT_H__
#define __LED_BOOT_H__

#include "hc32_ll.h"
#include "Gpio_io.h"

/*=============================================================================
 * Bootloader 升级指示 LED: 橙色 (R+Y 双通道同亮)，高电平点亮
 * 与 APP 工程配色一致 (led_module.c: 橙 = RED_YELLOW = R+Y 同亮)：
 *   LED1(M1): R=PC14 + Y=PC13
 *   LED2(M2): R=PA10 + Y=PH2
 * 状态机无外部接口，Led_Boot_Task 内部轮询 FlashDownload_GetState() 自动映射
 *============================================================================*/

/* 引脚定义: 每颗灯 = Y 通道 + R 通道 */
/* LED1 (M1) */
#define LED1_Y_PORT     GPIO_PORT_C         /* PC13 = M1_Y */
#define LED1_Y_PIN      GPIO_PIN_13
#define LED1_R_PORT     GPIO_PORT_C         /* PC14 = M1_R */
#define LED1_R_PIN      GPIO_PIN_14
/* LED2 (M2) */
#define LED2_Y_PORT     PH2_PORT            /* PH2 = M2_Y, 复用 Gpio_io.h 已有宏 */
#define LED2_Y_PIN      PH2_PIN
#define LED2_R_PORT     GPIO_PORT_A         /* PA10 = M2_R */
#define LED2_R_PIN      GPIO_PIN_10

/* 极性宏：高电平点亮 (与 APP 一致) */
#define LED_ON(port, pin)       GPIO_SET(port, pin)
#define LED_OFF(port, pin)      GPIO_RESET(port, pin)

/* 整灯控制: 橙色 = R+Y 双通道同亮/同灭 */
#define LED1_ON()       do { LED_ON(LED1_R_PORT, LED1_R_PIN); LED_ON(LED1_Y_PORT, LED1_Y_PIN); } while (0)
#define LED1_OFF()      do { LED_OFF(LED1_R_PORT, LED1_R_PIN); LED_OFF(LED1_Y_PORT, LED1_Y_PIN); } while (0)
#define LED2_ON()       do { LED_ON(LED2_R_PORT, LED2_R_PIN); LED_ON(LED2_Y_PORT, LED2_Y_PIN); } while (0)
#define LED2_OFF()      do { LED_OFF(LED2_R_PORT, LED2_R_PIN); LED_OFF(LED2_Y_PORT, LED2_Y_PIN); } while (0)

/* LED 状态枚举（由下载状态自动映射） */
typedef enum
{
    LED_BOOT_IDLE = 0,      /* 双灯 1000ms 闪烁 (1s 亮 1s 灭) */
    LED_BOOT_PROGRAMMING,   /* 双灯 50ms 闪烁 (亮 50ms + 灭 50ms) */
    LED_BOOT_DONE,          /* 双灯 500ms 闪烁 */
    LED_BOOT_FAIL           /* LED1 1000ms 闪烁, LED2 常灭 */
} LedBootState_t;

/* 双灯输出初始化(初始灭), 即进入 1s 慢闪 */
void Led_Boot_Init(void);

/* 非阻塞状态机轮询 (加在 UdsOta_Poll 的 1ms 门控块内) */
void Led_Boot_Task(void);

/* 双灯置灭 + 停用状态机, 跳转 APP 前调用 */
void Led_Boot_Shutdown(void);

#endif /* __LED_BOOT_H__ */
