/*******************************************
* 文件名: Led_Boot.c
* 功能: Bootloader 升级指示 LED 状态机 (PC13 / PH2, 低电平点亮)
* 说明: 非阻塞实现, 内部轮询 FlashDownload_GetState() 自动映射:
*       IDLE/PREPARING        -> 双灯灭 (刚进 bootloader, 未刷写)
*       READY/TRANSFERRING/VERIFYING -> 双灯 50ms 快闪 (刷写中)
*       COMPLETE              -> 双灯常亮 (刷写成功)
*       ERROR                 -> 双灯灭 (刷写失败)
*       LED_BOOT_STAY         -> 双灯常亮 (工装升级成功停留, 锁存)
*       时基: tickTimer_GetCount() (uint64 ms, 翻转法无回绕问题)
*******************************************/
#include "Led_Boot.h"
#include "flash_download.h"
#include "TickTimer.h"
#include "rtt_log.h"

/***************************** 静态变量 ***********************************/

static LedBootState_t s_ledState = LED_BOOT_IDLE;   /* 当前 LED 状态 */
static bool s_enabled = true;                       /* 状态机使能标志 (Shutdown 后停用) */
static uint64_t s_lastTick = 0;                     /* 上次切换电平的时刻 */
static bool s_blinkOn = false;                      /* 当前是否处于"亮"相位 */

/***************************** 内部函数 ***********************************/

/*
 * 状态切换: 重置相位 (s_lastTick = now), 按新状态设定初始电平, 打印可读日志
 * 闪灯态从"亮"开始; IDLE/FAIL 态双灯灭; STAY 态双灯常亮
 */
static void Led_Boot_SetState(LedBootState_t newState, uint64_t now)
{
    if (newState == s_ledState) {
        return;
    }
    s_ledState = newState;
    s_lastTick = now;

    if (newState == LED_BOOT_IDLE) {
        LED_OFF(LED_BL1_PORT, LED_BL1_PIN);
        LED_OFF(LED_BL2_PORT, LED_BL2_PIN);
        MAIN_D("LED state <-- IDLE(off)\r\n");
    }
    else if (newState == LED_BOOT_PROGRAMMING) {
        s_blinkOn = true;
        LED_ON(LED_BL1_PORT, LED_BL1_PIN);
        LED_ON(LED_BL2_PORT, LED_BL2_PIN);
        MAIN_D("LED state <-- PROGRAMMING(50ms blink)\r\n");
    }
    else if (newState == LED_BOOT_DONE) {
        LED_ON(LED_BL1_PORT, LED_BL1_PIN);
        LED_ON(LED_BL2_PORT, LED_BL2_PIN);
        MAIN_D("LED state <-- DONE(solid on)\r\n");
    }
    else if (newState == LED_BOOT_FAIL) {
        LED_OFF(LED_BL1_PORT, LED_BL1_PIN);
        LED_OFF(LED_BL2_PORT, LED_BL2_PIN);
        MAIN_D("LED state <-- FAIL(off)\r\n");
    }
    else { /* LED_BOOT_STAY */
        LED_ON(LED_BL1_PORT, LED_BL1_PIN);
        LED_ON(LED_BL2_PORT, LED_BL2_PIN);
        MAIN_D("LED state <-- STAY(solid on)\r\n");
    }
}

/*
 * 双灯按周期闪烁: 每过 period 翻转一次 (Nms 闪 = 亮 Nms + 灭 Nms)
 */
static void Led_Boot_Blink(uint32_t period, uint64_t now)
{
    if ((now - s_lastTick) < period) {
        return;
    }
    s_lastTick = now;
    s_blinkOn = !s_blinkOn;

    if (s_blinkOn) {
        LED_ON(LED_BL1_PORT, LED_BL1_PIN);
        LED_ON(LED_BL2_PORT, LED_BL2_PIN);
    } else {
        LED_OFF(LED_BL1_PORT, LED_BL1_PIN);
        LED_OFF(LED_BL2_PORT, LED_BL2_PIN);
    }
}

/***************************** 公开接口实现 *******************************/

/* 双灯输出初始化(灭), 即进入 IDLE 灭灯态 */
void Led_Boot_Init(void)
{
    Output_GPIO_Init(LED_BL1_PORT, LED_BL1_PIN, GPIO_INIT_HIGH);   /* PC13 初始灭 */
    Output_GPIO_Init(LED_BL2_PORT, LED_BL2_PIN, GPIO_INIT_HIGH);   /* PH2 初始灭 */

    s_ledState = LED_BOOT_IDLE;
    s_blinkOn = false;
    s_lastTick = tickTimer_GetCount();
    s_enabled = true;

    MAIN_D("LED state <-- IDLE(off)\r\n");
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

    now = tickTimer_GetCount();

    /* 停留态锁存: 双灯常亮, 不再随下载状态自动切换 (直至 Shutdown 熄灭) */
    if (s_ledState == LED_BOOT_STAY) {
        LED_ON(LED_BL1_PORT, LED_BL1_PIN);
        LED_ON(LED_BL2_PORT, LED_BL2_PIN);
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

    /* 失败→恢复自动处理: 重新 0x34 回 READY 自动回快闪, 会话超时回 IDLE 灭灯 */
    if (newState != s_ledState) {
        Led_Boot_SetState(newState, now);
    }

    /* 按当前状态驱动 */
    if (s_ledState == LED_BOOT_PROGRAMMING) {
        Led_Boot_Blink(50U, now);
    }
    /* IDLE / FAIL / DONE: 静态电平(灭/灭/常亮), 无需周期动作 */
}

/* 进入工装停留态: 双灯常亮 (锁存, 不再随下载状态自动切换) */
void Led_Boot_Stay(void)
{
    if (s_ledState == LED_BOOT_STAY) {
        return;
    }
    s_ledState = LED_BOOT_STAY;
    LED_ON(LED_BL1_PORT, LED_BL1_PIN);
    LED_ON(LED_BL2_PORT, LED_BL2_PIN);
    MAIN_D("LED state <-- STAY(solid on)\r\n");
}

/* 双灯置灭 + 停用状态机, 跳转 APP 前调用 */
void Led_Boot_Shutdown(void)
{
    s_enabled = false;
    LED_OFF(LED_BL1_PORT, LED_BL1_PIN);
    LED_OFF(LED_BL2_PORT, LED_BL2_PIN);
}
