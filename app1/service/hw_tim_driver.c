#include "hw_tim_driver.h"
#include "soft_timer.h"
#include "hc32_ll_tmr0.h"
#include "hc32_ll.h" // 替换为你芯片的库头文件

#define TMR0_UNIT                       (CM_TMR0_1)
#define TMR0_CLK                        (FCG2_PERIPH_TMR0_1)
#define TMR0_CH                         (TMR0_CH_B)
#define TMR0_CH_INT                     (TMR0_INT_CMP_B)
#define TMR0_CH_FLAG                    (TMR0_FLAG_CMP_B)
#define TMR0_INT_SRC                    (INT_SRC_TMR0_1_CMP_B)
#define TMR0_IRQn                       (INT006_IRQn)

/* Period = div / Clock freq * (Compare value + 1)   单位：s  即：1ms */
#define TMR0_CMP_VALUE                  (XTAL_VALUE / 4U / 1000U - 1U)

// 硬件TIM中断服务函数示例，以TIM1为例
#define PCLK_FREQ           (100000000UL)  // 100MHz
#define TMR0_DIV            (2UL)
#define TMR0_1MS_COUNT      ((PCLK_FREQ / TMR0_DIV) / 1000UL)  // = 100000
#define TMR0_1MS_CMP_VALUE  ((uint16_t)(TMR0_1MS_COUNT - 1UL)) // = 49999

//// 检查比较值是否超出16位范围
//#if (TMR0_1MS_CMP_VALUE > 0xFFFF)
//    #error "TMR0 compare value exceeds 16-bit limit! Please increase clock divider."
//#endif
// 硬件TIM中断服务函数示例，以TIM1为例
void TMR0_CompareIrqCallback(void)
{
    // 中断内调用软定时器滴答调度
    SoftTimer_TickISR();
    TMR0_ClearStatus(TMR0_UNIT, TMR0_CH_FLAG);
}

static void DEV_TMR0_Init(uint16_t tick_ms)
{
    stc_tmr0_init_t stcTmr0Init;
    stc_irq_signin_config_t stcIrqSignConfig;

    /* 1. 使能定时器外设时钟 */
    FCG_Fcg2PeriphClockCmd(TMR0_CLK, ENABLE);

    /* 2. TIMER0配置 - 使用100MHz PCLK */
    (void)TMR0_StructInit(&stcTmr0Init);
    stcTmr0Init.u32ClockSrc     = TMR0_CLK_SRC_INTERN_CLK;  // 内部同步时钟
    stcTmr0Init.u32ClockDiv     = TMR0_CLK_DIV2;       // 不分频
    stcTmr0Init.u32Func         = TMR0_FUNC_CMP;
    stcTmr0Init.u16CompareValue = TMR0_1MS_CMP_VALUE;  // 49999 (1ms)
    (void)TMR0_Init(TMR0_UNIT, TMR0_CH, &stcTmr0Init);

    /* 3. 配置定时器（同步时钟不需要延迟） */
    TMR0_HWStopCondCmd(TMR0_UNIT, TMR0_CH, ENABLE);
    TMR0_IntCmd(TMR0_UNIT, TMR0_CH_INT, ENABLE);

    /* 4. 清除可能的中断标志 */
//    TMR0_ClrIntFlag(TMR0_UNIT, TMR0_CH_INT);

    /* 5. 中断配置 */
    stcIrqSignConfig.enIntSrc    = TMR0_INT_SRC;
    stcIrqSignConfig.enIRQn      = TMR0_IRQn;
    stcIrqSignConfig.pfnCallback = &TMR0_CompareIrqCallback;
    (void)INTC_IrqSignIn(&stcIrqSignConfig);

    NVIC_ClearPendingIRQ(stcIrqSignConfig.enIRQn);
    NVIC_SetPriority(stcIrqSignConfig.enIRQn, DDL_IRQ_PRIO_DEFAULT);
    NVIC_EnableIRQ(stcIrqSignConfig.enIRQn);

    /* 6. 启动定时器 */
    TMR0_Start(TMR0_UNIT, TMR0_CH);

}

bool HW_TIM_Init(uint8_t tim_idx, uint16_t tick_ms)
{
    switch(tim_idx)
    {
        case 0:
            DEV_TMR0_Init(tick_ms);
            break;
        default:
            return false;
    }
    return true;
}
