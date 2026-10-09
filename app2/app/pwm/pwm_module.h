#ifndef __PWM_MODULE_H__
#define __PWM_MODULE_H__

#include "hc32_ll_tmra.h"
#include "hc32_ll_gpio.h"
#include "hc32_ll_fcg.h"

/* TMRA config (must match TMRA_1 in power module) */
#define PWM_FREQ_HZ      10000UL
#define PWM_TIMER_DIV    TMRA_CLK_DIV8
#define PWM_PERIOD_VAL   2500            /* 25MHz / 2500 = 10kHz */

#define PWM_PERIOD_VAL_0 (0)
#define PWM_PERIOD_MAX   (PWM_PERIOD_VAL - 1)  /* 2499 */
/* 电机实例 */
typedef enum {
    MOTOR1 = 0,
    MOTOR2  = 1,
} pwm_motor_t;

/* Channel type */
typedef enum {
    CH_PWM = 0,
    CH_IO  = 1,
} pwm_ch_type_t;

/* Channel type */
typedef enum {
    CH_STATE_INVALID = 0,
    CH_STATE_VALID   = 1
} pwm_ch_state_t;

/* IO pin config */
typedef struct {
    uint32_t port;
    uint16_t pin;
    uint8_t  polarity;   /* 1=active-high(GPIO_Reset=on), 0=active-low(GPIO_Set=on) */
} pwm_io_t;

/* Single channel config */
typedef struct {
    pwm_ch_type_t    type;
    pwm_io_t         io;
    /* PWM only */
    uint32_t         tim_clk;
    CM_TMRA_TypeDef *TMRAx;
    uint32_t         tmra_ch;
    uint16_t         func;
    uint8_t          is_breaked;
    uint32_t         time_count;
    // uint32_t         last_time;
    // uint32_t         current_time;
} pwm_ch_cfg_t;

/* Channel index */
typedef enum {
    CH_M1_LU = 0,   /* Motor1, low-side U  (PWM, PB03, TMRA6_CH5) */
    CH_M1_LV = 1,   /* Motor1, low-side V  (PWM, PB04, TMRA6_CH6) */
    CH_M2_LU = 2,   /* Motor2, low-side U  (PWM, PB05, TMRA6_CH7) */
    CH_M2_LV = 3,   /* Motor2, low-side V  (PWM, PB06, TMRA6_CH8) */
    CH_M1_HU = 4,   /* Motor1, high-side U (IO,  PB07) */
    CH_M1_HV = 5,   /* Motor1, high-side V (IO,  PA15) */
    CH_M2_HU = 6,   /* Motor2, high-side U (IO,  PA06) */
    CH_M2_HV = 7,   /* Motor2, high-side V (IO,  PA07) */

	PWM_CH_MAX = 8,
} pwm_channel_t;

extern pwm_ch_cfg_t CFG_HW[PWM_CH_MAX];

/* Timer-level API (init & start once, syncs TMRA_6 + TMRA_1) */
void pwm_timer_init(void);
/*----------------------------------------------------------------------------*
 * 外部接口：启动定时器（仅对PWM通道有效）
 * 参数：handle – 实例句柄
 * 返回值：0 – 成功，-1 – 参数错误
 *----------------------------------------------------------------------------*/
int8_t pwm_timera_6_start(void);
int8_t pwm_output_enable(pwm_channel_t ch);

/* Channel-level API */
void pwm_ch_init(void);
void pwm_ch_start(pwm_channel_t ch);
void pwm_ch_stop(pwm_channel_t ch);
void pwm_ch_forced(pwm_channel_t ch,uint8_t state);
void pwm_ch_set(pwm_channel_t ch, uint32_t duty);   /* IO: nop */
void pwm_ch_break(pwm_motor_t motor_idx);
void io_ch_on(pwm_channel_t ch);
void io_ch_off(pwm_channel_t ch);

#endif /* __PWM_MODULE_H__ */
