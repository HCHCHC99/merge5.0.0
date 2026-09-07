
#ifndef __SYS_TICK_H
#define __SYS_TICK_H

#include <stdint.h>
#include "sys_config.h"

//void mySysTick_Init(void);
uint32_t mySysTick_Get(void);
//void mySysTick_Inc(void);  // 在定时器中断中调用

#endif
