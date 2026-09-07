
/**
 * @file led_hw_table.c
 * @brief LED硬件配置表
 *
 * 硬件映射：
 *   LED1: M1_R=PC14, M1_G=PC15, M1_Y=PC13, 高电平有效
 *   LED2: M2_R=PA10, M2_G=PA09, M2_Y=PH02, 高电平有效
 */

#include "led.h"
#include "hc32_ll_gpio.h"

// ==============================
// 硬件配置表
// ==============================

//M1_R  PC14  高电平 有效
//M1_G  PC15  高电平 有效
//M1_Y  PC13  高电平 有效

//M2_R  PA10  高电平 有效
//M2_G  PA09  高电平 有效
//M2_Y  PH02  高电平 有效

const LED_HWConfig_t LED_HW_TABLE[] = {
    // LED1 - M1
    {
        .gpio_r = GPIO_PORT_C,
        .pin_r = GPIO_PIN_14,
        .gpio_g = GPIO_PORT_C,
        .pin_g = GPIO_PIN_15,
        .gpio_y = GPIO_PORT_C,
        .pin_y = GPIO_PIN_13,
        .active_level = 1,      // 高电平有效
    },
    // LED2 - M2
    {
        .gpio_r = GPIO_PORT_A,
        .pin_r = GPIO_PIN_10,
        .gpio_g = GPIO_PORT_A,
        .pin_g = GPIO_PIN_09,
        .gpio_y = GPIO_PORT_H,
        .pin_y = GPIO_PIN_02,
        .active_level = 1,      // 高电平有效
    },
};
