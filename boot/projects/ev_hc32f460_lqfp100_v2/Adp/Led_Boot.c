/*******************************************
* 文件名: Led_Boot.c
* 功能: Bootloader 升级指示 LED 状态机 — 橙色 (R+Y 双通道同亮, 高电平点亮)
* 引脚: LED1(M1): R=PC14 + Y=PC13 | LED2(M2): R=PA10 + Y=PH2
*       与 APP 工程配色一致 (led_module.c: 橙 = RED_YELLOW = R+Y 同亮)
* 说明: 非阻塞实现, 无外部接口, 内部轮询 FlashDownload_GetState() 自动映射:
*       IDLE/PREPARING  -> 双灯 1000ms 闪烁
*       READY/TRANSFERRING/VERIFYING -> 双灯 50ms 闪烁
*       COMPLETE        -> 双灯 500ms 闪烁
*       ERROR           -> LED1 1000ms 闪烁, LED2 常灭
*       时基: tickTimer_GetCount() (uint64 ms, 翻转法无回绕问题)
*******************************************/
#include "Led_Boot.h"
#include "flash_download.h"
#include "TickTimer.h"
#include "rtt_log.h"

/***************************** 静态变量 ***********************************/

static LedBootState_t s_ledState = LED_BOOT_IDLE;   /* 当前 LED 状态 */
static bool s_enabled = false;                      /* 状态机使能标志 (Shutdown 后停用) */
static uint64_t s_lastTick = 0;                     /* 上次切换电平的时刻 */
static bool s_blinkOn = false;                      /* 当前是否处于"亮"相位 */

/***************************** 内部函数 ***********************************/

/*
 * 状态切换: 重置相位 (s_lastTick = now), 按新状态设定初始电平, 打印可读日志
 * 闪灯态从"亮"开始; FAIL 态 PC13 从亮开始、PH2 置灭, 避免残留电平
 */
static void Led_Boot_SetState(LedBootState_t newState, uint64_t now)
{
    if (newState == s_ledState) {
        return;
    }
    s_ledState = newState;
    s_lastTick = now;

    if (newState == LED_BOOT_FAIL) {
        /* LED1 常亮开始闪烁, LED2 置灭 */
        s_blinkOn = true;
        LED1_ON();
        LED2_OFF();
        MAIN_D("LED state <-- FAIL(LED1 1000ms blink, LED2 off)\r\n");
    }
    else if (newState == LED_BOOT_PROGRAMMING) {
        s_blinkOn = true;
        LED1_ON();
        LED2_ON();
        MAIN_D("LED state <-- PROGRAMMING(50ms blink)\r\n");
    }
    else if (newState == LED_BOOT_DONE) {
        s_blinkOn = true;
        LED1_ON();
        LED2_ON();
        MAIN_D("LED state <-- DONE(500ms blink)\r\n");
    }
    else {
        s_blinkOn = true;
        LED1_ON();
        LED2_ON();
        MAIN_D("LED state <-- IDLE(1000ms blink)\r\n");
    }
}

/*
 * 双灯按周期闪烁: 每过 period 翻转一次 (Nms 闪 = 亮 Nms + 灭 Nms)
 * FAIL 态仅 LED1 闪烁, LED2 保持常灭
 */
static void Led_Boot_Blink(uint32_t period, uint64_t now)
{
    if ((now - s_lastTick) < period) {
        return;
    }
    s_lastTick = now;
    s_blinkOn = !s_blinkOn;

    if (s_ledState == LED_BOOT_FAIL) {
        /* LED1 闪, LED2 常灭 */
        if (s_blinkOn) {
            LED1_ON();
        } else {
            LED1_OFF();
        }
        LED2_OFF();
    }
    else {
        /* 双灯同步闪烁 */
        if (s_blinkOn) {
            LED1_ON();
            LED2_ON();
        } else {
            LED1_OFF();
            LED2_OFF();
        }
    }
}

/***************************** 公开接口实现 *******************************/

/* 双灯输出初始化(初始灭: 高电平有效, 初始拉低), 即进入 1s 慢闪 */
void Led_Boot_Init(void)
{
    Output_GPIO_Init(LED1_R_PORT, LED1_R_PIN, GPIO_INIT_LOW);   /* PC14 = M1_R 初始灭 */
    Output_GPIO_Init(LED1_Y_PORT, LED1_Y_PIN, GPIO_INIT_LOW);   /* PC13 = M1_Y 初始灭 */
    Output_GPIO_Init(LED2_R_PORT, LED2_R_PIN, GPIO_INIT_LOW);   /* PA10 = M2_R 初始灭 */
    Output_GPIO_Init(LED2_Y_PORT, LED2_Y_PIN, GPIO_INIT_LOW);   /* PH2  = M2_Y 初始灭 */

    s_ledState = LED_BOOT_IDLE;
    s_blinkOn = false;
    s_lastTick = tickTimer_GetCount();
    s_enabled = true;

    MAIN_D("LED state <-- IDLE(1000ms blink)\r\n");
}

/* 非阻塞状态机轮询 (加在 UdsOta_Poll 的 1ms 门控块内) */
void Led_Boot_Task(void)
{
    FlashDownloadState_t dlState;
    LedBootState_t newState;
    uint64_t now;

    if (!s_enabled) {
        return;
    }

    /* 轮询下载状态机, 自动映射 LED 状态 */
    dlState = FlashDownload_GetState();
    if ((dlState == FW_UPDATE_IDLE) || (dlState == FW_UPDATE_PREPARING)) {
        newState = LED_BOOT_IDLE;
    }
    else if ((dlState == FW_UPDATE_READY) || (dlState == FW_UPDATE_TRANSFERRING) ||
             (dlState == FW_UPDATE_VERIFYING)) {
        newState = LED_BOOT_PROGRAMMING;
    }
    else if (dlState == FW_UPDATE_COMPLETE) {
        newState = LED_BOOT_DONE;
    }
    else {
        newState = LED_BOOT_FAIL;
    }

    now = tickTimer_GetCount();

    /* 失败→恢复自动处理: TBOX 重新 0x34 回 READY 自动回快闪, 会话超时回 IDLE 回慢闪 */
    if (newState != s_ledState) {
        Led_Boot_SetState(newState, now);
    }

    /* 按当前状态闪烁 */
    if (s_ledState == LED_BOOT_IDLE) {
        Led_Boot_Blink(1000U, now);
    }
    else if (s_ledState == LED_BOOT_PROGRAMMING) {
        Led_Boot_Blink(50U, now);
    }
    else if (s_ledState == LED_BOOT_DONE) {
        Led_Boot_Blink(500U, now);
    }
    else {
        Led_Boot_Blink(1000U, now);
    }
}

/* 双灯置灭 + 停用状态机, 跳转 APP 前调用 */
void Led_Boot_Shutdown(void)
{
    s_enabled = false;
    LED1_OFF();
    LED2_OFF();
}
