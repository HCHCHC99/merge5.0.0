#ifndef __LOG_RTT_H
#define __LOG_RTT_H

#include <stdint.h>
#include "log_config.h"
#include "SEGGER_RTT.h"

//====================================================================
// 1. 日志等级定义
//====================================================================
#define LOG_LEVEL_DEBUG    0
#define LOG_LEVEL_INFO     1
#define LOG_LEVEL_WARN     2
#define LOG_LEVEL_ERROR    3
#define LOG_LEVEL_FATAL    4

//====================================================================
// 2. 【多通道定义】给每个功能模块分配一个通道号  目前只支持一个通道0
//====================================================================
typedef enum {
    LOG_CH_MAIN    = 0,    // 主程序
    LOG_CH_KEY     = 0,    // key 模块
    LOG_CH_LED     = 0,    // LED 模块
    LOG_CH_MOTOR   = 0,    // 电机
    LOG_CH_COMM    = 0,    // 通信
    LOG_CH_POWER   = 0,    // 电源模块
    LOG_CH_MAX             // 通道总数
} LogChannel_t;

//====================================================================
// 3. RTT 颜色控制码（J-Link RTT Viewer 支持）
//====================================================================
#define COLOR_RED       "\033[31m"
#define COLOR_GREEN     "\033[32m"
#define COLOR_YELLOW    "\033[33m"
#define COLOR_BLUE      "\033[34m"
#define COLOR_CYAN      "\033[36m"
#define COLOR_CLEAR     "\033[0m"  // 清除颜色

//====================================================================
// 4. 全局日志等级配置
//====================================================================
#ifndef LOG_LEVEL_CONFIG
#define LOG_LEVEL_CONFIG   LOG_LEVEL_DEBUG
#endif

#ifndef LOG_ENABLE
#define LOG_ENABLE 1
#endif

//====================================================================
// 5. 核心打印宏（修复版）
//====================================================================
#define LOG(channel, level, color, tag, fmt, ...) \
    do { \
        if (level >= LOG_LEVEL_CONFIG) { \
            /*SEGGER_RTT_SetTerminal(channel);*/\
            SEGGER_RTT_printf(channel, color "[%s] " fmt COLOR_CLEAR "\r\n", tag, ##__VA_ARGS__); \
        } \
    } while(0)

// ====================== 分级日志接口 ======================
#if LOG_ENABLE

// ---------------------- 主程序 MAIN (通道0) ----------------------

// ---------------------- MAIN 详细分级--------------------
#define MAIN_D(fmt, ...)  LOG(LOG_CH_MAIN, LOG_LEVEL_DEBUG, COLOR_CYAN,   "MAIN", fmt, ##__VA_ARGS__)
#define MAIN_I(fmt, ...)  LOG(LOG_CH_MAIN, LOG_LEVEL_INFO,  COLOR_GREEN,  "MAIN", fmt, ##__VA_ARGS__)
#define MAIN_W(fmt, ...)  LOG(LOG_CH_MAIN, LOG_LEVEL_WARN,  COLOR_YELLOW, "MAIN", fmt, ##__VA_ARGS__)
#define MAIN_E(fmt, ...)  LOG(LOG_CH_MAIN, LOG_LEVEL_ERROR, COLOR_RED,    "MAIN", fmt, ##__VA_ARGS__)

		// ---------------------- 通道1：USB ----------------------
#define KEY_D(fmt, ...)   LOG(LOG_CH_KEY, LOG_LEVEL_DEBUG, COLOR_CYAN,    "KEY", fmt, ##__VA_ARGS__)
#define KEY_I(fmt, ...)   LOG(LOG_CH_KEY, LOG_LEVEL_INFO,  COLOR_GREEN,   "KEY", fmt, ##__VA_ARGS__)
#define KEY_W(fmt, ...)   LOG(LOG_CH_KEY, LOG_LEVEL_WARN,  COLOR_YELLOW,  "KEY", fmt, ##__VA_ARGS__)
#define KEY_E(fmt, ...)   LOG(LOG_CH_KEY, LOG_LEVEL_ERROR, COLOR_RED,     "KEY", fmt, ##__VA_ARGS__)

// ---------------------- 通道2：传感器 SENSOR ----------------------
#define LED_D(fmt, ...) LOG(LOG_CH_LED, LOG_LEVEL_DEBUG, COLOR_CYAN,    "LED", fmt, ##__VA_ARGS__)
#define LED_I(fmt, ...) LOG(LOG_CH_LED, LOG_LEVEL_INFO,  COLOR_GREEN,   "LED", fmt, ##__VA_ARGS__)
#define LED_W(fmt, ...) LOG(LOG_CH_LED, LOG_LEVEL_WARN,  COLOR_YELLOW,  "LED", fmt, ##__VA_ARGS__)
#define LED_E(fmt, ...) LOG(LOG_CH_LED, LOG_LEVEL_ERROR, COLOR_RED,     "LED", fmt, ##__VA_ARGS__)

// ---------------------- 通道3：电机 MOTOR ----------------------
#define MOTOR_D(fmt, ...) LOG(LOG_CH_MOTOR, LOG_LEVEL_DEBUG, COLOR_CYAN,    "MOTOR", fmt, ##__VA_ARGS__)
#define MOTOR_I(fmt, ...) LOG(LOG_CH_MOTOR, LOG_LEVEL_INFO,  COLOR_GREEN,   "MOTOR", fmt, ##__VA_ARGS__)
#define MOTOR_W(fmt, ...) LOG(LOG_CH_MOTOR, LOG_LEVEL_WARN,  COLOR_YELLOW,  "MOTOR", fmt, ##__VA_ARGS__)
#define MOTOR_E(fmt, ...) LOG(LOG_CH_MOTOR, LOG_LEVEL_ERROR, COLOR_RED,     "MOTOR", fmt, ##__VA_ARGS__)

// ---------------------- 通道4：通信 COMM ----------------------
#define COMM_D(fmt, ...)  LOG(LOG_CH_COMM, LOG_LEVEL_DEBUG, COLOR_CYAN,    "COMM", fmt, ##__VA_ARGS__)
#define COMM_I(fmt, ...)  LOG(LOG_CH_COMM, LOG_LEVEL_INFO,  COLOR_GREEN,   "COMM", fmt, ##__VA_ARGS__)
#define COMM_W(fmt, ...)  LOG(LOG_CH_COMM, LOG_LEVEL_WARN,  COLOR_YELLOW,  "COMM", fmt, ##__VA_ARGS__)
#define COMM_E(fmt, ...)  LOG(LOG_CH_COMM, LOG_LEVEL_ERROR, COLOR_RED,     "COMM", fmt, ##__VA_ARGS__)

// ---------------------- 通道5：UI ----------------------
#define POWER_D(fmt, ...)    LOG(LOG_CH_POWER, LOG_LEVEL_DEBUG, COLOR_CYAN,    "POWER", fmt, ##__VA_ARGS__)
#define POWER_I(fmt, ...)    LOG(LOG_CH_POWER, LOG_LEVEL_INFO,  COLOR_GREEN,   "POWER", fmt, ##__VA_ARGS__)
#define POWER_W(fmt, ...)    LOG(LOG_CH_POWER, LOG_LEVEL_WARN,  COLOR_YELLOW,  "POWER", fmt, ##__VA_ARGS__)
#define POWER_E(fmt, ...)    LOG(LOG_CH_POWER, LOG_LEVEL_ERROR, COLOR_RED,     "POWER", fmt, ##__VA_ARGS__)

// ====================== 通用日志接口 ======================
#define LOG_ERROR(fmt, ...)  LOG(LOG_CH_MAIN, LOG_LEVEL_ERROR, COLOR_RED,    "ERROR", fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)   LOG(LOG_CH_MAIN, LOG_LEVEL_WARN,  COLOR_YELLOW, "WARN",  fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)   LOG(LOG_CH_MAIN, LOG_LEVEL_INFO,  COLOR_GREEN,  "INFO",  fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...)  LOG(LOG_CH_MAIN, LOG_LEVEL_DEBUG, COLOR_CYAN,   "DEBUG", fmt, ##__VA_ARGS__)

#else
#define LOG_ERROR(...)
#define LOG_WARN(...)
#define LOG_INFO(...)
#define LOG_DEBUG(...)

#define MAIN_D(...)
#define MAIN_I(...)
#define MAIN_W(...)
#define MAIN_E(...)

#define KEY_D(...)
#define KEY_I(...)
#define KEY_W(...)
#define KEY_E(...)

#define LED_D(...)
#define LED_I(...)
#define LED_W(...)
#define LED_E(...)

#define MOTOR_D(...)
#define MOTOR_I(...)
#define MOTOR_W(...)
#define MOTOR_E(...)

#define COMM_D(...)
#define COMM_I(...)
#define COMM_W(...)
#define COMM_E(...)

#define POWER_D(...)
#define POWER_I(...)
#define POWER_W(...)
#define POWER_E(...)
#endif

//====================================================================
// 6. 初始化函数
//====================================================================
void Log_Init(void);

#endif
