/**
 * @file sensor_pot.c
 * @brief 电位计位置传感器驱动
 */

#include "sensor_pot.h"
#include "hal_adc.h"

/* 内部函数 */
static uint8_t    Pot_Init(void);
static int32_t    Pot_ReadRaw(void);
static int32_t    Pot_ReadMm(void);
static float      Pot_ReadPercent(void);
static void       Pot_Calibrate(void);

/* 注册接口 */
const PosSensorOps_t PotOps = {
    .init          = Pot_Init,
    .read_raw      = Pot_ReadRaw,
    .read_mm       = Pot_ReadMm,
    .read_percent  = Pot_ReadPercent,
    .calibrate     = Pot_Calibrate,
};

static uint8_t Pot_Init(void)
{
    /* 后续可添加ADC初始化 */
    return 0;
}

static int32_t Pot_ReadRaw(void)
{
    /* 实际：return Hal_ADC_Read(POT_ADC_CH); */
    return 2048;
}

static int32_t Pot_ReadMm(void)
{
    /* 实际：根据行程换算 mm */
    return 50;
}

static float Pot_ReadPercent(void)
{
    /* 0~100% 行程 */
    return 50.0f;
}

static void Pot_Calibrate(void)
{
    /* 校准逻辑 */
}
