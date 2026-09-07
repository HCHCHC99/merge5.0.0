#include "power_manager.h"
#include "log_rtt.h"

void Test_Power(void)
{
    LOG_INFO("======= 电源模块测试 =======");
    Power_Init();
    Power_Task();

    const PowerInfo_t* info = Power_GetInfo();
    LOG_INFO("电压: %d mV", info->voltage_now);
    LOG_INFO("状态: %d", info->state);
    LOG_INFO("======= 测试完成 =======");
}
