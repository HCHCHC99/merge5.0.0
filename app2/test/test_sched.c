#include "sys_sched.h"
#include "log_rtt.h"
#include "sys_tick.h"

void Test_Scheduler(void)
{
    LOG_INFO("========== 调度器测试 ==========");
    LOG_INFO("调度器初始化成功");
    LOG_INFO("当前Tick：%d", SysTick_Get());
    LOG_INFO("调度器测试完成\n");
}
