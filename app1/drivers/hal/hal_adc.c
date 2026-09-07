#include "hal_adc.h"

#if HAL_ENABLE_ADC

// extern void Hw_ADC_Init(const HalADC_t *adc);
// extern uint16_t Hw_ADC_Read(const HalADC_t *adc);

void Hal_ADC_Init(const HalADC_t *adc)
{
    // Hw_ADC_Init(adc);
}

uint16_t Hal_ADC_Read(const HalADC_t *adc)
{
    // return Hw_ADC_Read(adc);
		  return 0;
}

#endif
