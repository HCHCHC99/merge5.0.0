#include "hal_pwm.h"

#if HAL_ENABLE_PWM

// extern void Hw_PWM_Init(const HalPWM_t *pwm, uint32_t freq_hz);
// extern void Hw_PWM_SetDuty(const HalPWM_t *pwm, uint8_t duty);
// extern void Hw_PWM_Start(const HalPWM_t *pwm);
// extern void Hw_PWM_Stop(const HalPWM_t *pwm);

void Hal_PWM_Init(const HalPWM_t *pwm, uint32_t freq_hz)
{
    // Hw_PWM_Init(pwm, freq_hz);
}

void Hal_PWM_SetDuty(const HalPWM_t *pwm, uint8_t duty)
{
    // Hw_PWM_SetDuty(pwm, duty);
}

void Hal_PWM_Start(const HalPWM_t *pwm)
{
    // Hw_PWM_Start(pwm);
}

void Hal_PWM_Stop(const HalPWM_t *pwm)
{
    // Hw_PWM_Stop(pwm);
}

#endif
