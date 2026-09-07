/**
 * pwm_module.c - Unified channel driver (PWM & IO)
 *
 * Timer management:
 *   pwm_timer_init()  — config TMRA_6, sync with TMRA_1, init PWM channels
 *   pwm_timer_start() — start both timers synchronously (called once)
 *
 * IO:  polarity=0(GPIO_Set=on), polarity=1(GPIO_Reset=on)
 * PWM: active-low (larger duty = longer low = more power)
 */

#include "pwm_module.h"
#include "main.h"

/*（注：PB3,PB4,PA13,PA14,PA15默认是烧录接口,需要禁止烧录功能才能用作GPIO）
问题：PA15和PB4作为一组上下桥，失能时间应该错开，不然电压高时上电会直接烧板子
*/

/*----------------------------------------------------------------------------*
 * 内部：硬件初始化 -- 通用GPIO（方向输出，数字属性）
 * 参数：io_ch – IO通道配置指针
 * 返回值：无
 *----------------------------------------------------------------------------*/
static void pwm_gpio_init(const pwm_io_t *io_ch)
{
    stc_gpio_init_t stcGpioInit;

    if (io_ch == NULL) return;

    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PinDir = PIN_DIR_OUT;
    stcGpioInit.u16PinDrv = PIN_HIGH_DRV;
    stcGpioInit.u16PinAttr = PIN_ATTR_DIGITAL;

    if (io_ch->polarity)
    {
        stcGpioInit.u16PinState = PIN_STAT_RST;
    }else{
        stcGpioInit.u16PinState = PIN_STAT_SET;
    }

    (void)GPIO_Init(io_ch->port, io_ch->pin, &stcGpioInit);
}

/*----------------------------------------------------------------------------*
 * 内部：硬件初始化 -- 定时器PWM通道（含GPIO功能复用）
 * 说明：配置定时器为PWM输出，并设置引脚复用功能
 * 参数：ch – PWM通道配置指针
 * 返回值：无
 *----------------------------------------------------------------------------*/
static void pwm_timera_init(const pwm_ch_cfg_t *ch)
{
    stc_tmra_init_t    stcTmraInit;
    stc_tmra_pwm_init_t stcTmraPwmInit;

    if (ch == NULL) return;

    // 使能定时器时钟
    FCG_Fcg2PeriphClockCmd(ch->tim_clk, ENABLE);

    // 配置引脚复用功能
    GPIO_SetFunc(ch->io.port, ch->io.pin, ch->func);

    // 定时器基础配置
    stcTmraInit.u8CountSrc = TMRA_CNT_SRC_SW;
    stcTmraInit.sw_count.u8ClockDiv = TMRA_CLK_DIV8;
    stcTmraInit.sw_count.u8CountMode = TMRA_MD_SAWTOOTH;
    stcTmraInit.sw_count.u8CountDir = TMRA_DIR_UP;
    stcTmraInit.u32PeriodValue = PWM_PERIOD_VAL - 1;   // 周期值 = 计数最大值
    (void)TMRA_Init(ch->TMRAx, &stcTmraInit);

    // PWM 输出极性配置（低电平有效）
    stcTmraPwmInit.u16StartPolarity = TMRA_PWM_HIGH;
    stcTmraPwmInit.u16StopPolarity = TMRA_PWM_HIGH;
    stcTmraPwmInit.u16CompareMatchPolarity = TMRA_PWM_HIGH;
    stcTmraPwmInit.u16PeriodMatchPolarity = TMRA_PWM_LOW;

    (void)TMRA_PWM_Init(ch->TMRAx, ch->tmra_ch, &stcTmraPwmInit);

    TMRA_PWM_SetForcePolarity(ch->TMRAx, ch->tmra_ch, TMRA_PWM_FORCE_HIGH);     //RL-260513_add：初始化后关闭PWM，强制置高电平
}

/*----------------------------------------------------------------------------*
 * 内部：根据通道类型初始化一个PWM/IO通道
 * 参数：ch – 通道配置指针
 * 返回值：无
 *----------------------------------------------------------------------------*/
static void pwm_channel_init(const pwm_ch_cfg_t *ch)
{
    if (ch == NULL) return;

    // 1. 初始化 GPIO（方向输出）
    pwm_gpio_init(&ch->io);

    // 2. 如果是 PWM 类型，还需配置定时器
    if (ch->type == CH_PWM) {
        pwm_timera_init(ch);
    } else if (ch->type == CH_IO) {
        // 对于纯 IO 通道，仅设置引脚复用功能（如有需要）
        if (ch->func != 0) {
            GPIO_SetFunc(ch->io.port, ch->io.pin, ch->func);
        }
    }
}

/* ---- Channel-level ---- */

void pwm_ch_init(void)
{
    const pwm_ch_cfg_t *cfg;

	GPIO_SetDebugPort(GPIO_PIN_TDI | GPIO_PIN_SWO, DISABLE);
	DDL_DelayMS(2U);
	GPIO_SetDebugPort(GPIO_PIN_TRST, DISABLE);

	for (uint8_t ch = 0; ch < PWM_CH_MAX; ch++) {
		cfg = &CFG_HW[ch];
        pwm_channel_init(cfg);
    }
}

/*----------------------------------------------------------------------------*
 * 外部接口：启动定时器（仅对PWM通道有效）
 * 参数：handle – 实例句柄
 * 返回值：0 – 成功，-1 – 参数错误
 *----------------------------------------------------------------------------*/
int8_t pwm_timera_6_start(void)
{
	// 与 TIM1 同步启动（这几个都是一个unit，就调用一遍）
    TMRA_SyncStartCmd(CM_TMRA_6, ENABLE);

	TMRA_Start(CM_TMRA_1);      //RL-260514_add：同步功能 开启TimA-1后，timA-6自动开始

    return 0;
}

int8_t pwm_output_enable(pwm_channel_t ch)
{
	const pwm_ch_cfg_t *cfg;
	cfg = &CFG_HW[ch];

	if (cfg == NULL) return -1;

    TMRA_PWM_OutputCmd(cfg->TMRAx, cfg->tmra_ch,ENABLE);  //RL-260513_add：PWM通道打开需要使能 在这里统一使能，其他地方不在单独使能，屏蔽掉
	return 0;
}

void pwm_ch_start(pwm_channel_t ch)
{
	pwm_ch_cfg_t *cfg;
    if (ch >= PWM_CH_MAX) return;
    cfg = &CFG_HW[ch];

    cfg->is_breaked = 0;

	if (cfg->type == CH_PWM) {
        TMRA_PWM_SetForcePolarity(cfg->TMRAx, cfg->tmra_ch, TMRA_PWM_FORCE_INVD);       //L-260513_add：打开PWM之前，解除强制电平
    } else {
        // 如果是 IO ,则开启通道
		if (cfg->io.polarity == 0) {
            GPIO_ResetPins(cfg->io.port, cfg->io.pin);
        } else {
            GPIO_SetPins(cfg->io.port, cfg->io.pin);
        }
    }
}

void pwm_ch_stop(pwm_channel_t ch)
{
    const pwm_ch_cfg_t *cfg;
    if (ch >= PWM_CH_MAX) return;
    cfg = &CFG_HW[ch];

    if (cfg->type == CH_PWM) {
        TMRA_PWM_SetForcePolarity(cfg->TMRAx,cfg->tmra_ch, TMRA_PWM_FORCE_HIGH);    //L-260513_add：关闭-PWM强制置高电平
    } else {
        if (cfg->io.polarity == 0) {
            GPIO_SetPins(cfg->io.port, cfg->io.pin);
        } else {
            GPIO_ResetPins(cfg->io.port, cfg->io.pin);
        }
    }
}

void pwm_ch_break(pwm_motor_t motor_idx)
{
    if (MOTOR1 == motor_idx)
    {
        pwm_ch_cfg_t *cfg_m1_hu;
        pwm_ch_cfg_t *cfg_m1_hv;
        cfg_m1_hu = &CFG_HW[CH_M1_HU];
        cfg_m1_hv = &CFG_HW[CH_M1_HV];

        if (cfg_m1_hu->is_breaked == 0)
        {
            cfg_m1_hu->is_breaked = 1;
            cfg_m1_hu->time_count = 0;
        }

        cfg_m1_hu->time_count++;

        if (cfg_m1_hu->io.polarity == 0) {
            GPIO_SetPins(cfg_m1_hu->io.port, cfg_m1_hu->io.pin);       // 关闭 上桥臂，若低电平有效，置位
        } else {
            GPIO_ResetPins(cfg_m1_hu->io.port, cfg_m1_hu->io.pin);
        }

        if (cfg_m1_hv->io.polarity == 0) {
            GPIO_SetPins(cfg_m1_hv->io.port, cfg_m1_hv->io.pin);
        } else {
            GPIO_ResetPins(cfg_m1_hv->io.port, cfg_m1_hv->io.pin);
        }

        if ( cfg_m1_hu->time_count > 2 ) // 死区时间  200us x3 = 600 us
        {
            cfg_m1_hu->time_count = 3;
            // 开启下桥臂 强制输出
            pwm_ch_forced(CH_M1_LV,1);
            pwm_ch_forced(CH_M1_LU,1);
        }

    }else if (MOTOR2 == motor_idx)
    {
        pwm_ch_cfg_t *cfg_m2_hu;
        pwm_ch_cfg_t *cfg_m2_hv;
        cfg_m2_hu = &CFG_HW[CH_M2_HU];
        cfg_m2_hv = &CFG_HW[CH_M2_HV];

        if (cfg_m2_hu->is_breaked == 0)
        {
            cfg_m2_hu->is_breaked = 1;
            cfg_m2_hu->time_count = 0;
        }

        cfg_m2_hu->time_count++;

        if (cfg_m2_hu->io.polarity == 0) {
            GPIO_SetPins(cfg_m2_hu->io.port, cfg_m2_hu->io.pin);           // 关闭 上桥臂，若低电平有效，置位
        } else {
            GPIO_ResetPins(cfg_m2_hu->io.port, cfg_m2_hu->io.pin);
        }

        if (cfg_m2_hv->io.polarity == 0) {
            GPIO_SetPins(cfg_m2_hv->io.port, cfg_m2_hv->io.pin);           // 关闭 上桥臂，若低电平有效，置位
        } else {
            GPIO_ResetPins(cfg_m2_hv->io.port, cfg_m2_hv->io.pin);
        }

        if ( cfg_m2_hu->time_count > 2 ) // 死区时间  200us x3 = 600 us
        {
            cfg_m2_hu->time_count = 3;
            // 开启下桥臂 强制输出
            pwm_ch_forced(CH_M2_LU,1);
            pwm_ch_forced(CH_M2_LV,1);
        }
    }
}

void pwm_ch_forced(pwm_channel_t ch,uint8_t state)
{
    const pwm_ch_cfg_t *cfg;
    if (ch >= PWM_CH_MAX) return;
    cfg = &CFG_HW[ch];

    if (cfg->type == CH_PWM) {

        if (state == 0)   // 设置为 无效电平
        {
            TMRA_PWM_SetForcePolarity(cfg->TMRAx,cfg->tmra_ch, TMRA_PWM_FORCE_HIGH);    // PWM 高电平 无效
        }else             // 设置为 有效电平
        {
            TMRA_PWM_SetForcePolarity(cfg->TMRAx,cfg->tmra_ch, TMRA_PWM_FORCE_LOW);    // PWM 低电平 有效
        }
    }
}

void io_ch_on(pwm_channel_t ch)
{
    const pwm_ch_cfg_t *cfg;
    if (ch >= PWM_CH_MAX) return;
    cfg = &CFG_HW[ch];

    if (cfg->type == CH_IO) {
        if (cfg->io.polarity == 0) {
            GPIO_ResetPins(cfg->io.port, cfg->io.pin);
        } else {
            GPIO_SetPins(cfg->io.port, cfg->io.pin);
        }
    };
}

void io_ch_off(pwm_channel_t ch)
{
    const pwm_ch_cfg_t *cfg;
    if (ch >= PWM_CH_MAX) return;
    cfg = &CFG_HW[ch];

    if (cfg->type == CH_IO) {
        if (cfg->io.polarity == 0) {
            GPIO_SetPins(cfg->io.port, cfg->io.pin);
        } else {
            GPIO_ResetPins(cfg->io.port, cfg->io.pin);
        }
    };
}

void pwm_ch_set(pwm_channel_t ch, uint32_t duty)
{
    const pwm_ch_cfg_t *cfg;
    if (ch >= PWM_CH_MAX) return;
    cfg = &CFG_HW[ch];

    if (cfg->type == CH_IO) {

        if (duty == 0)      // 设置为 无效电平
        {
            if (cfg->io.polarity == 0) {
                GPIO_SetPins(cfg->io.port, cfg->io.pin);
            } else {
                GPIO_ResetPins(cfg->io.port, cfg->io.pin);
            }
        }else{             // 设置为  有效电平
            if (cfg->io.polarity == 0) {
                GPIO_ResetPins(cfg->io.port, cfg->io.pin);
            } else {
                GPIO_SetPins(cfg->io.port, cfg->io.pin);
            }
        }

        return;
    };

    if (duty > PWM_PERIOD_MAX) duty = PWM_PERIOD_MAX;
    TMRA_SetCompareValue(cfg->TMRAx, cfg->tmra_ch, duty);
    TMRA_SetCompareValue(CM_TMRA_1, TMRA_CH1, duty/2);              //260702_add:增加adc采样对比值的

}
