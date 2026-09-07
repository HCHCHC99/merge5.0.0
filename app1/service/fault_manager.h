#ifndef __FAULT_MANAGER_H
#define __FAULT_MANAGER_H

#include "sys_config.h"
#include <stdint.h>

#if APP_ENABLE_FAULT_MANAGE

// 初始化故障管理
void Fault_Init(void);

// 故障管理任务
void Fault_Task(void);

// 上报故障
void Fault_Report(uint8_t fault_code);

// 清除故障
void Fault_Clear(uint8_t fault_code);

// 获取当前故障码
uint8_t Fault_GetCurrent(void);

#endif

#endif
