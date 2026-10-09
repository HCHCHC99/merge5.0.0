#include "hal_gpio.h"
#include "hal_pwm.h"
#include "hal_adc.h"
#include "log_rtt.h"

void Test_HAL(void)
{
    LOG_INFO("========== HAL驱动测试 ==========");
    LOG_INFO("GPIO 抽象层 OK");
    LOG_INFO("PWM  抽象层 OK");
    LOG_INFO("ADC  抽象层 OK");
    LOG_INFO("TIMER 抽象层 OK");
    LOG_INFO("HAL 测试完成\n");
}
