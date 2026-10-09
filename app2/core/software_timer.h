#ifndef __SOFTWARE_TIMER_H
#define __SOFTWARE_TIMER_H

#include <stdint.h>

#ifndef NULL
  #define NULL                                            0
#endif

/* 配置项：静态最大定时器个数 */
#define SWT_MAX_INSTANCE    8U

/* 定时器运行模式 */
typedef enum
{
    SWT_ONE_SHOT    = 0U,   // 单次触发
    SWT_PERIODIC    = 1U    // 周期循环
}SwTimerMode_t;

/* 定时回调函数原型 */
typedef void (*SwTimerCallback)(void);

/* 单个定时器结构体 */
typedef struct
{
    uint8_t         enFlag;         // 使能标志：1启用 0停止
    SwTimerMode_t   workMode;       // 工作模式
    uint32_t        tickStart;      // 启动时刻系统Tick(ms)
    uint32_t        periodMs;       // 定时时长ms
    SwTimerCallback pFuncCb;        // 回调函数
}SwTimerObj_t;

/* API */
/**
 * @brief 申请空闲定时器索引
 * @retval 0~SWT_MAX_INSTANCE-1：有效索引；0xFF：无空闲
 */
uint8_t SwTimer_Create(void);

/**
 * @brief 配置定时器参数
 * @param idx:定时器编号
 * @param ms:定时毫秒
 * @param mode:单次/周期
 * @param cb:回调函数指针
 */
void SwTimer_Config(uint8_t idx,uint32_t ms,SwTimerMode_t mode,SwTimerCallback cb);

/**
 * @brief 启动定时器
 */
void SwTimer_Start(uint8_t idx);

/**
 * @brief 停止定时器
 */
void SwTimer_Stop(uint8_t idx);

/**
 * @brief 删除定时器(清空配置)
 */
void SwTimer_Delete(uint8_t idx);

/**
 * @brief 定时轮询处理，放主循环while(1)中循环调用
 */
void SwTimer_PollTask(void);

extern uint32_t sysTickMs;
#endif
