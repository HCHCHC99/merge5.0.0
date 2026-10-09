/********************************文件说明*************************************
*文件名: power_module.c
*作者: zhou
*版本: V1.0.0
*功能简介: 电源模块.包括电压、电流采样
*备注: 无
*修改履历: 去掉内部滤波，增加环形缓冲区
*****************************************************************************/
/** 文件包含 **/
#include "power_module.h"
#include "pwm_module.h"
#include "app_power.h"
#include "ctrl_current.h"
#include "pid_common.h"
#include "main.h"
#include <string.h>
#include "act_state.h"
#include <math.h>

// 声明外部句柄（用于在init中区分）
extern power_handle_t power_vm;
extern power_handle_t power_m1;
extern power_handle_t power_m2;

/** 内部 -- 环形缓冲区实例 **/
static power_ring_buffer_t ring_buffer_vm;
static power_ring_buffer_t ring_buffer_m1;
static power_ring_buffer_t ring_buffer_m2;

#define CURR_FILTER_ALPHA  0.05f   // fc≈40Hz @ 200μs PWM周期 260626_add
float g_filter_alpha = 0.05f;

//260703_add:电流环形数组
/* 电流反馈均值滤波 — 环形缓冲区 260703_add */
#define CURR_FILT_BUF_SIZE  5   // 环形数组宽度，可根据实际需要调整（
typedef struct {
    float buffer[CURR_FILT_BUF_SIZE];
    uint8_t idx;      // 写指针
    uint8_t count;    // 当前有效样本数（≤ BUF_SIZE）
} curr_filt_buf_t;
static curr_filt_buf_t curr_filt_m1;   // 推杆 1 电流滤波缓冲
static curr_filt_buf_t curr_filt_m2;   // 推杆 2 电流滤波缓冲

/** 内部 -- GPIO初始化 -- 模拟通道 **/
static void power_gpio_init(const adc_channel_t *ch)
{
    stc_gpio_init_t stcGpioInit;

    if (ch == NULL) return;

    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PinAttr = PIN_ATTR_ANALOG;
    (void)GPIO_Init(ch->port, ch->pin, &stcGpioInit);
}

/** 内部 -- ADC基本配置初始化 **/
static int8_t power_adc_init(CM_ADC_TypeDef *ADCx, const power_hw_cfg_t *hw_cfg)
{
    stc_adc_init_t stcAdc;

    if (ADCx == CM_ADC1)
    {
        FCG_Fcg3PeriphClockCmd(FCG3_PERIPH_ADC1, ENABLE);
    } else if (ADCx == CM_ADC2) {
        FCG_Fcg3PeriphClockCmd(FCG3_PERIPH_ADC2, ENABLE);
    }

    (void)ADC_StructInit(&stcAdc);
    stcAdc.u16ScanMode     = hw_cfg->scan_mode;
    stcAdc.u16Resolution   = hw_cfg->resolution;
    stcAdc.u16DataAlign    = hw_cfg->data_align;

    if (ADC_Init(ADCx, &stcAdc) != LL_OK) return -1;

    return 0;
}

/** 内部 -- 使能通道序列 **/
static void power_channel_enable(CM_ADC_TypeDef *ADCx, const adc_channel_t *ch)
{
    if (ch == NULL) return;

    if (ch->seq == ADC_SEQ_A)
    {
        ADC_ChCmd(ADCx, ADC_SEQ_A, ch->channel, ENABLE);
    } else {
        ADC_ChCmd(ADCx, ADC_SEQ_B, ch->channel, ENABLE);
    }
}

/** 内部（特殊） -- timer初始化 **/
static void spc_timera_init(void)
{
    stc_tmra_init_t stcTmraInit;

    FCG_Fcg2PeriphClockCmd(FCG2_PERIPH_TMRA_1, ENABLE);

    stcTmraInit.u8CountSrc = TMRA_CNT_SRC_SW;
    stcTmraInit.sw_count.u8ClockDiv = TMRA_CLK_DIV8;
    stcTmraInit.sw_count.u8CountMode = TMRA_MD_SAWTOOTH;
    stcTmraInit.sw_count.u8CountDir = TMRA_DIR_UP;
    stcTmraInit.u32PeriodValue = PWM_PERIOD_VAL - 1;
    (void)TMRA_Init(CM_TMRA_1, &stcTmraInit);

    TMRA_SetCompareValue(CM_TMRA_1, TMRA_CH1, 300);
    TMRA_EventCmd(CM_TMRA_1, TMRA_EVT_CMP_CH1, ENABLE);
}

/** 内部（特殊） -- 硬件触发源选择 **/
static void power_adc_trg(CM_ADC_TypeDef *ADCx, uint8_t seq, const adc_ht_t *ht)
{
    if (ht == NULL) return;

    FCG_Fcg0PeriphClockCmd(FCG0_PERIPH_AOS, ENABLE);
    AOS_SetTriggerEventSrc(ht->aos_trg, ht->aos_evt_src);
    ADC_TriggerConfig(ADCx, seq, ht->hardtrig_src);
}

/** 内部 -- 中断配置 **/
static void adc_int_cfg(CM_ADC_TypeDef *ADCx, en_int_src_t int_src, const adc_int_t *adc_int)
{
    stc_irq_signin_config_t stcIrq;

    if (adc_int == NULL) return;

    if (ADCx == CM_ADC1)
    {
        stcIrq.enIntSrc = int_src;
        stcIrq.enIRQn = adc_int->adc_int_irqn;
        stcIrq.pfnCallback = adc_int->adc_int_callback;
        (void)INTC_IrqSignIn(&stcIrq);

        NVIC_ClearPendingIRQ(adc_int->adc_int_irqn);
        NVIC_SetPriority(adc_int->adc_int_irqn, adc_int->adc_int_pri);
        NVIC_EnableIRQ(adc_int->adc_int_irqn);

        if (int_src == INT_SRC_ADC1_EOCA)
        {
            ADC_IntCmd(ADCx, ADC_INT_EOCA, ENABLE);
        }
        else if (int_src == INT_SRC_ADC1_EOCB)
        {
            ADC_IntCmd(ADCx, ADC_INT_EOCB, ENABLE);
        }
    }
}

/** 外部 -- 模块硬件初始化 **/
int32_t power_module_init(power_handle_t *handle, const power_cfg_t *cfg)
{
    if (handle == NULL || cfg == NULL || cfg->hw == NULL) return -1;

    handle->cfg = cfg;
    handle->initialized = 0;

    memset(&handle->power_upd, 0, sizeof(power_upd_t));

    // 根据句柄地址关联对应的环形缓冲区
    if (handle == &power_vm) {
        handle->ring_buffer = &ring_buffer_vm;
        memcpy(handle->power_upd.name, "VM", 3);
        handle->power_upd.idx = 0;
    } else if (handle == &power_m1) {
        handle->ring_buffer = &ring_buffer_m1;
        memcpy(handle->power_upd.name, "M1", 3);
        handle->power_upd.idx = 1;
    } else if (handle == &power_m2) {
        handle->ring_buffer = &ring_buffer_m2;
        memcpy(handle->power_upd.name, "M2", 3);
        handle->power_upd.idx = 2;
    } else {
        static power_ring_buffer_t dynamic_buffer;
        handle->ring_buffer = &dynamic_buffer;
    }

    // 清空环形缓冲区
    if (handle->ring_buffer) {
        memset(handle->ring_buffer, 0, sizeof(power_ring_buffer_t));
    }

    const power_hw_cfg_t *hw = cfg->hw;

    if (hw->en_voltage)    power_gpio_init(&hw->vol_ch);
    if (hw->en_current)    power_gpio_init(&hw->cur_ch);
    if (hw->en_polarity)   power_gpio_init(&hw->pol_ch);

    if (power_adc_init(hw->ADCx, hw) != 0) return -2;

    if (hw->en_voltage)    power_channel_enable(hw->ADCx, &hw->vol_ch);
    if (hw->en_current)    power_channel_enable(hw->ADCx, &hw->cur_ch);
    if (hw->en_polarity)   power_channel_enable(hw->ADCx, &hw->pol_ch);

    if (hw->en_ht_seqa)    power_adc_trg(hw->ADCx, ADC_SEQ_A, &hw->ht_seqa);
    if (hw->en_ht_seqb)    power_adc_trg(hw->ADCx, ADC_SEQ_B, &hw->ht_seqb);

    if (hw->en_int_seqa)   adc_int_cfg(hw->ADCx, INT_SRC_ADC1_EOCA, &hw->int_seqa);
    if (hw->en_int_seqb)   adc_int_cfg(hw->ADCx, INT_SRC_ADC1_EOCB, &hw->int_seqb);

    if (hw->ht_seqb.en_timera_1) spc_timera_init();

    handle->initialized = 1;
    return 0;
}

/** 外部 -- ADC模块启动 **/
int32_t power_module_start(power_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) return -1;

    const power_hw_cfg_t *hw = handle->cfg->hw;

    if (HARD_TRIG_ENABLE == hw->en_ht_seqa)
    {
        ADC_TriggerCmd(hw->ADCx, ADC_SEQ_A, ENABLE);
    }

    if (HARD_TRIG_ENABLE == hw->en_ht_seqb)
    {
        ADC_TriggerCmd(hw->ADCx, ADC_SEQ_B, ENABLE);
    }

    return 0;
}

/** 外部 -- ADC采样启动 -- 软件 **/
int32_t adc_sw_start(power_handle_t *handle)
{
    if (handle == NULL || !handle->initialized) return -1;
    return ADC_Start(handle->cfg->hw->ADCx);
}

uint16_t g_PWMSet = 2000;
void driver_motor_run(Motor_t *motor){

    Motor_t *p_motor = motor;

    uint8_t index = 0;

    if ( p_motor == &mySystem.axis[0].motor)
    {
        index = 0;
    }else if (p_motor == &mySystem.axis[1].motor)
    {
        index = 1;
    }else{
        return;
    }

    // index == 0  或 index == 1
    if (motor->dir == MOT_DIR_FORWARD )
    {
        if (motor->dir_last != motor->dir)
        {
            mySystem.axis[index].curr_ctrl.forward_is_forced = true;
            mySystem.axis[index].curr_ctrl.reverse_is_forced = true;
            pwm_ch_break((pwm_motor_t)(MOTOR1+index));
            // mySystem.axis[index].swt_enable = false;
            mySystem.axis[index].swt_start = false;

            //260708——add：清空电流环PID
            PID_ResetOutput(&mySystem.axis[index].curr_ctrl);

        }else{

            pwm_ch_forced((pwm_channel_t)(CH_M1_LU+index*2),(uint8_t)CH_STATE_INVALID);               //   LU 强制完全关闭
            mySystem.axis[index].curr_ctrl.reverse_is_forced = true;

            io_ch_off((pwm_channel_t)(CH_M1_HV+index*2));                                             //   HV 关闭

            // 非阻塞延时
            if (!mySystem.axis[index].swt_enable)
            {
                mySystem.axis[index].swt_enable = true;
            }

            if (mySystem.axis[index].swt_enable)
            {
                if (!mySystem.axis[index].swt_start)
                {
                    mySystem.axis[index].swt_start = true;
                    mySystem.axis[index].swt_count = 0;
                }else
                {
                    mySystem.axis[index].swt_count++;
                }
            }

            if (mySystem.axis[index].swt_count > 3 )
            {
                mySystem.axis[index].swt_count = 3;

                io_ch_on((pwm_channel_t)(CH_M1_HU+index*2));                                     //   HU 开启

                // LV 开启
                if (mySystem.axis[index].curr_ctrl.out > PWM_PERIOD_MAX)  // pwm占空比  需要到达 100%
                {
                    // 标记 pwm 占空比 强制 100%
                    mySystem.axis[index].curr_ctrl.forward_is_forced = true;

                    // 强制 输出 pwm 占空比 100%
                    // 低电平有效,必须强制输出为 低
                    pwm_ch_forced((pwm_channel_t)(CH_M1_LV+index*2),(uint8_t)CH_STATE_VALID);
                }else{

                    if (mySystem.axis[index].curr_ctrl.forward_is_forced)
                    {
                        // 解除强制输出
                        pwm_ch_start((pwm_channel_t)(CH_M1_LV+index*2));
                        mySystem.axis[index].curr_ctrl.forward_is_forced = false;
                    }

                    pwm_ch_set((pwm_channel_t)(CH_M1_LV+index*2), mySystem.axis[index].curr_ctrl.out);
//                    pwm_ch_set((pwm_channel_t)(CH_M1_LV+index*2), g_PWMSet);
                }
            }

        }

    }else if (motor->dir == MOT_DIR_REVERSE)
    {
        if (motor->dir_last != motor->dir)
        {
            mySystem.axis[index].curr_ctrl.forward_is_forced = true;
            mySystem.axis[index].curr_ctrl.reverse_is_forced = true;
            pwm_ch_break((pwm_motor_t)(MOTOR1+index));
            // mySystem.axis[index].swt_enable = false;
            mySystem.axis[index].swt_start = false;
        }else{

            pwm_ch_forced((pwm_channel_t)(CH_M1_LV+index*2),(uint8_t)CH_STATE_INVALID);               //   LV 强制完全关闭
            mySystem.axis[index].curr_ctrl.reverse_is_forced = true;

            io_ch_off((pwm_channel_t)(CH_M1_HU+index*2));                                             //   HU 关闭

            // 非阻塞延时
            if (!mySystem.axis[index].swt_enable)
            {
                mySystem.axis[index].swt_enable = true;
            }

            if (mySystem.axis[index].swt_enable)
            {
                if (!mySystem.axis[index].swt_start)
                {
                    mySystem.axis[index].swt_start = true;
                    mySystem.axis[index].swt_count = 0;
                }else
                {
                    mySystem.axis[index].swt_count++;
                }
            }

            if (mySystem.axis[index].swt_count > 3 )
            {
                mySystem.axis[index].swt_count = 3;

                io_ch_on((pwm_channel_t)(CH_M1_HV+index*2));                                     //   HV 开启

                // LU 开启
                if (mySystem.axis[index].curr_ctrl.out > PWM_PERIOD_MAX)  // pwm占空比  需要到达 100%
                {
                    // 标记 pwm 占空比 强制 100%
                    mySystem.axis[index].curr_ctrl.forward_is_forced = true;

                    // 强制 输出 pwm 占空比 100%
                    // 低电平有效,必须强制输出为 低
                    pwm_ch_forced((pwm_channel_t)(CH_M1_LU+index*2),(uint8_t)CH_STATE_VALID);
                }else{

                    if (mySystem.axis[index].curr_ctrl.forward_is_forced)
                    {
                        // 解除强制输出
                        pwm_ch_start((pwm_channel_t)(CH_M1_LU+index*2));
                        mySystem.axis[index].curr_ctrl.forward_is_forced = false;
                    }
                    pwm_ch_set((pwm_channel_t)(CH_M1_LU+index*2), mySystem.axis[index].curr_ctrl.out);
//                    pwm_ch_set((pwm_channel_t)(CH_M1_LU+index*2), g_PWMSet);
                }
            }

        }

    }else{
        mySystem.axis[index].curr_ctrl.forward_is_forced = true;
        mySystem.axis[index].curr_ctrl.reverse_is_forced = true;
        pwm_ch_break((pwm_motor_t)(MOTOR1+index));
        // mySystem.axis[index].swt_enable = false;
        mySystem.axis[index].swt_start = false;
    }

    motor->dir_last = motor->dir;
}

//260718_add:到上下断电点判断
//260718_add:到上下断电点判断
static void power_module_limit(uint8_t idx)
{
    static uint16_t s_limit_time_cnt= 0 ;
    static uint16_t s_limit_cnt= 0 ;
    static float  s_last_pos[2] = {0} ;

    //260806_修改到断电点判断
    //1、位置在时间段内不变化；2、电流环输出有值
    if(mySystem.axis[idx].curr_ctrl.out  > 2200)  //有电流输出，占空比大于某个值
    {
        if(s_limit_time_cnt > 500)   //0.2ms*N=Xms检测一次位置
        {
            if(fabs(s_last_pos[idx] - mySystem.axis[idx].pos_ctrl.fdb) < 0.1)
            {
                s_limit_cnt++;
                if(s_limit_cnt > 10)
                {
                    s_limit_cnt = 0;
                    if(ACT_MODE_CALIB_MODE == mySystem.axis[idx].mode)
                    {
                        //发送  EVT_CALIB_AUTO_CALIB_BACK   事件
                        EventGroup_Send(mySystem.axis[idx].evt_calib, EVT_CALIB_AUTO_CALIB_BACK);
                    }
                    else
                    {
                        //置堵转标志
                        mySystem.axis[idx].act_stall_flag = true;
                    }
                }
            }
            else
            {
				if (s_limit_cnt > 0)
				{
					s_limit_cnt --;
				}

            }
            s_limit_time_cnt = 0;
            s_last_pos[idx] = mySystem.axis[idx].pos_ctrl.fdb;
        }
        s_limit_time_cnt  ++;
    }
}

float g_CurOut[2000] = {0};
float g_CurErr[2000] = {0};
float g_CurFkb[2000] = {0};
float g_CurSet[2000] = {0};
float g_CurSourceFkb[2000] = {0};
uint16_t g_PwmSet = 1000;
/** 外部 -- 数据处理（在电机的pwm的200us中断中调用，只做原始数据采集和存储）**/
void power_module_process(power_handle_t *handle)
{
//    static uint16_t j,k ,m=0 ;

    float voltage_raw = 0, current_raw = 0;

    if (handle == NULL || !handle->initialized) return;

    const power_hw_cfg_t *hw = handle->cfg->hw;

    // 采集电压
    if (hw->en_voltage) {
        uint16_t adc = ADC_GetValue(hw->ADCx, hw->vol_ch.channel);
        voltage_raw = (adc * hw->verf / (1 << hw->adc_res)) * hw->vol_gain + hw->vol_offset;
        handle->power_upd.voltage = voltage_raw;  // 保存最新值
    }

    // 采集电流
    if (hw->en_current) {
        uint16_t adc = ADC_GetValue(hw->ADCx, hw->cur_ch.channel);
        current_raw = (adc * hw->verf / (1 << hw->adc_res)) * hw->cur_gain + hw->cur_offset;
        handle->power_upd.current = (uint16_t)current_raw;  // 保存最新值

        // 调用 电流环计算
        if (handle->power_upd.idx == 1 || handle->power_upd.idx == 2)     // idx == 1  推杆1  idx == 2  推杆2
        {
            // 如果 电机 是使能状态
            if (mySystem.axis[handle->power_upd.idx-1].motor.enable)
            {

                if (mySystem.axis[handle->power_upd.idx-1].curr_ctrl.enable)
                {

//////                    //260626_add
//                    float raw = current_raw;
//                    float filtered = mySystem.axis[handle->power_upd.idx-1].curr_fdb_filtered;
//                    filtered = filtered + g_filter_alpha * (raw - filtered);
//                    mySystem.axis[handle->power_upd.idx-1].curr_fdb_filtered = filtered;
//                    mySystem.axis[handle->power_upd.idx-1].curr_ctrl.fdb = filtered;  // 替代原 current_raw

                    //260703_add:尝试电流均值滤波
                    uint8_t ax_idx = handle->power_upd.idx - 1;   // 0 or 1
                    curr_filt_buf_t *buf = (ax_idx == 0) ? &curr_filt_m1 : &curr_filt_m2;

                    // 1. 写入环形缓冲区
                    buf->buffer[buf->idx] = current_raw;
                    buf->idx = (buf->idx + 1) % CURR_FILT_BUF_SIZE;
                    if (buf->count < CURR_FILT_BUF_SIZE) {
                        buf->count++;
                    }

                    // 2. 计算平均值
                    float sum = 0.0f;
                    for (uint8_t i = 0; i < buf->count; i++) {
                        sum += buf->buffer[i];
                    }
                    float avg = sum / (float)buf->count;

                    // 3. 赋值给电流环反馈
                    mySystem.axis[ax_idx].curr_ctrl.fdb = avg;

                    //mySystem.axis[handle->power_upd.idx-1].curr_ctrl.fdb = current_raw;   //原本的current_raw

                    // 执行 电流环PID 算法
                    PID_Calc(&mySystem.axis[handle->power_upd.idx-1].curr_param,
                            &mySystem.axis[handle->power_upd.idx-1].curr_ctrl,
                            mySystem.axis[handle->power_upd.idx-1].curr_ctrl.set,
                            mySystem.axis[handle->power_upd.idx-1].curr_ctrl.fdb);

////                    //260702_test:直接设置pwm
////                    mySystem.axis[handle->power_upd.idx-1].curr_ctrl.out = g_PwmSet;
//

//                    // 需要判定 堵转
//                    // 判定逻辑
//                    // 1.未到达 目标位置
//                    // 2.速度 ＜ 阈值以下
//                    // 3.占空比 ＜ 阈值以下
                    //260622——增加启动屏蔽：motor enable 后前 500ms 跳过判定
                    if (mySystem.axis[handle->power_upd.idx-1].startup_mask_count <
                        mySystem.axis[handle->power_upd.idx-1].startup_mask_thr)
                    {
                        mySystem.axis[handle->power_upd.idx-1].startup_mask_count++;
                        // 屏蔽期间，清零 stall/success 计数，避免屏蔽结束后残留值误判
                        mySystem.axis[handle->power_upd.idx-1].stall_count = 0;
                        mySystem.axis[handle->power_upd.idx-1].success_count = 0;
                    }
                    else
                    {
                        if ((mySystem.axis[handle->power_upd.idx-1].pos_ctrl.err > mySystem.axis[handle->power_upd.idx-1].pos_ctrl.stall_thr ||
                            mySystem.axis[handle->power_upd.idx-1].pos_ctrl.err < -mySystem.axis[handle->power_upd.idx-1].pos_ctrl.stall_thr) &&
                            //mySystem.axis[handle->power_upd.idx-1].spd_ctrl.fdb < mySystem.axis[handle->power_upd.idx-1].spd_ctrl.stall_thr &&
                            mySystem.axis[handle->power_upd.idx-1].curr_ctrl.fdb  > mySystem.axis[handle->power_upd.idx-1].curr_ctrl.over_current_thr )
                            //mySystem.axis[handle->power_upd.idx-1].curr_ctrl.out < mySystem.axis[handle->power_upd.idx-1].curr_ctrl.stall_thr)
                        {
                            if (mySystem.axis[handle->power_upd.idx-1].stall_count >= mySystem.axis[handle->power_upd.idx-1].stall_count_thr)
                            {
                                mySystem.axis[handle->power_upd.idx-1].motor.dir = MOT_DIR_NONE;
                                mySystem.axis[handle->power_upd.idx-1].motor.enable = false;
                                //260718_add:堵转判断，分标定模式、正常模式下
                                //标定模式下，发送堵转 事件给标定状态机
                                if(ACT_MODE_CALIB_MODE == mySystem.axis[handle->power_upd.idx-1].mode)
                                {
                                    //发送  EVT_CALIB_AUTO_CALIB_BACK   事件
                                   EventGroup_Send(mySystem.axis[handle->power_upd.idx-1].evt_calib, EVT_CALIB_AUTO_CALIB_BACK);
                                }
                                else
                                {
                                    //260622_Rl_add: 置堵转标志、发事件
                                   mySystem.axis[handle->power_upd.idx-1].act_stall_flag = true;
                                }
                                //260708_Rl_add: 清空计数
                                mySystem.axis[handle->power_upd.idx-1].stall_count = 0;
                            }
                            mySystem.axis[handle->power_upd.idx-1].stall_count++;
                        }else{
                                //mySystem.axis[handle->power_upd.idx-1].stall_count = 0;
                                //260708_Rl_add: 改成减减
                                if( mySystem.axis[handle->power_upd.idx-1].stall_count > 0)
                                {
                                    mySystem.axis[handle->power_upd.idx-1].stall_count--;
                                }
                            }

                        //260718_add:
                        power_module_limit(handle->power_upd.idx-1);

                        //260622_Rl_add: 调用堵转函数
//                        if(g_ActStallFlag ==1)
//                        {
                            //260708——add：清空电流环PID
                            //PID_ResetOutput(&mySystem.axis[handle->power_upd.idx-1].curr_ctrl);
                            act_babk_handler();
//
                    }
					// 判定 到达目标位置
					if (fabsf(mySystem.axis[handle->power_upd.idx-1].pos_ctrl.set - mySystem.axis[handle->power_upd.idx-1].pos_ctrl.fdb) <  mySystem.axis[handle->power_upd.idx-1].pos_ctrl.stall_thr)
						 //mySystem.axis[handle->power_upd.idx-1].spd_ctrl.fdb < mySystem.axis[handle->power_upd.idx-1].spd_ctrl.stall_thr )
						//mySystem.axis[handle->power_upd.idx-1].curr_ctrl.out < mySystem.axis[handle->power_upd.idx-1].curr_ctrl.stall_thr)
					{
						if (mySystem.axis[handle->power_upd.idx-1].success_count >= mySystem.axis[handle->power_upd.idx-1].success_count_thr)
						{
							//260718_add:到位置判断，分标定模式、正常模式下
							//标定模式下，到位置，发送事件给标定状态机
							if(ACT_MODE_CALIB_MODE == mySystem.axis[handle->power_upd.idx-1].mode)
							{
								//发送 EVT_CALIB_AUTO_DONE  事件
								EventGroup_Send(mySystem.axis[handle->power_upd.idx-1].evt_calib, EVT_CALIB_AUTO_DONE);
							}
							else
							{
								mySystem.axis[handle->power_upd.idx-1].motor.dir = MOT_DIR_NONE;
								mySystem.axis[handle->power_upd.idx-1].motor.enable = false;
								//260702_rl_add:到位之后电机停止、三环失能
								EventGroup_Send(mySystem.axis[handle->power_upd.idx-1].motor.evt_mot, EVT_MOT_STOP);     //推杆停止
								mySystem.axis[handle->power_upd.idx-1].curr_ctrl.enable = false;
								mySystem.axis[handle->power_upd.idx-1].spd_ctrl.enable = false;
								mySystem.axis[handle->power_upd.idx-1].pos_ctrl.enable  =false;
								//260708——add：三环失能后，清空电流环PID
								PID_ResetOutput(&mySystem.axis[handle->power_upd.idx-1].curr_ctrl);
								//260714_add:发送结合/分离 成功事件
								if(mySystem.axis[handle->power_upd.idx-1].sm_act.cur_state == ACT_STATE_SEPARATE ||
								   mySystem.axis[handle->power_upd.idx-1].sm_act.cur_state == ACT_STATE_SEPARATE_TRY_1 ||
								   mySystem.axis[handle->power_upd.idx-1].sm_act.cur_state == ACT_STATE_SEPARATE_TRY_2 )
								{
									EventGroup_Send(mySystem.axis[handle->power_upd.idx-1].evt_act, EVT_ACT_SEPARATE_SUCCESS);
								}
								else if(mySystem.axis[handle->power_upd.idx-1].sm_act.cur_state == ACT_STATE_COMBINE   ||
										mySystem.axis[handle->power_upd.idx-1].sm_act.cur_state == ACT_STATE_COMBINE_TRY_1 ||
										mySystem.axis[handle->power_upd.idx-1].sm_act.cur_state == ACT_STATE_COMBINE_TRY_2 )
								{
									EventGroup_Send(mySystem.axis[handle->power_upd.idx-1].evt_act, EVT_ACT_COMBINE_SUCCESS);
								}
							}
						}
						mySystem.axis[handle->power_upd.idx-1].success_count++;
					}else
					{
						if(mySystem.axis[handle->power_upd.idx-1].success_count > 0)
						mySystem.axis[handle->power_upd.idx-1].success_count--;
					}
                }else{
                    mySystem.axis[handle->power_upd.idx-1].motor.dir = MOT_DIR_NONE;
                }

                // driver_motor_run( handle->power_upd.idx-1,mySystem.axis[handle->power_upd.idx-1].motor.dir);
                driver_motor_run(&mySystem.axis[handle->power_upd.idx-1].motor);

            }else{                                                         // 电机 未使能

                mySystem.axis[handle->power_upd.idx-1].motor.dir = MOT_DIR_NONE;
                mySystem.axis[handle->power_upd.idx-1].startup_mask_count = 0;  //260625_RL:add：清零启动屏蔽计数
                // mySystem.axis[handle->power_upd.idx-1].motor.enable = false;
                // driver_motor_run( handle->power_upd.idx-1,MOT_DIR_NONE);
                driver_motor_run(&mySystem.axis[handle->power_upd.idx-1].motor);
            }
        }
    }

    // 写入环形缓冲区
    power_buffer_write(handle, voltage_raw, current_raw);
}

/** 外部 -- 获取最新数据 **/
int32_t power_module_get_data(const power_handle_t *handle, float *volt, float *curr, uint8_t *polarity)
{
    if (handle == NULL || !handle->initialized) return -1;

    if (volt) *volt = handle->power_upd.voltage;
    if (curr) *curr = (float)handle->power_upd.current;
    if (polarity) *polarity = handle->power_upd.polarity;

    return 0;
}

/** 环形缓冲区 -- 写入数据 **/
int32_t power_buffer_write(power_handle_t *handle, float voltage, float current)
{
    if (handle == NULL || handle->ring_buffer == NULL) return -1;

    power_ring_buffer_t *rb = handle->ring_buffer;

    // 写入电压数据
    rb->voltage_buffer[rb->voltage_write_idx] = voltage;
    rb->voltage_write_idx = (rb->voltage_write_idx + 1) % POWER_DATA_BUFFER_SIZE;
    if (rb->voltage_count < POWER_DATA_BUFFER_SIZE) {
        rb->voltage_count++;
    }

    // 写入电流数据
    rb->current_buffer[rb->current_write_idx] = current;
    rb->current_write_idx = (rb->current_write_idx + 1) % POWER_DATA_BUFFER_SIZE;
    if (rb->current_count < POWER_DATA_BUFFER_SIZE) {
        rb->current_count++;
    }

    return 0;
}

/** 获取缓冲区电压数据数量 **/
uint16_t power_buffer_get_voltage_count(const power_handle_t *handle)
{
    if (handle == NULL || handle->ring_buffer == NULL) return 0;
    return handle->ring_buffer->voltage_count;
}

/** 获取缓冲区电流数据数量 **/
uint16_t power_buffer_get_current_count(const power_handle_t *handle)
{
    if (handle == NULL || handle->ring_buffer == NULL) return 0;
    return handle->ring_buffer->current_count;
}

/** 读取电压缓冲区数据 **/
int32_t power_buffer_read_voltage(power_handle_t *handle, float *data, uint16_t len, uint16_t *read_count)
{
    uint16_t i;
    uint16_t count = 0;

    if (handle == NULL || handle->ring_buffer == NULL || data == NULL) return -1;

    power_ring_buffer_t *rb = handle->ring_buffer;

    count = (len < rb->voltage_count) ? len : rb->voltage_count;

    for (i = 0; i < count; i++) {
        data[i] = rb->voltage_buffer[rb->voltage_read_idx];
        rb->voltage_read_idx = (rb->voltage_read_idx + 1) % POWER_DATA_BUFFER_SIZE;
        rb->voltage_count--;
    }

    if (read_count) *read_count = count;

    return 0;
}

/** 读取电流缓冲区数据 **/
int32_t power_buffer_read_current(power_handle_t *handle, float *data, uint16_t len, uint16_t *read_count)
{
    uint16_t i;
    uint16_t count = 0;

    if (handle == NULL || handle->ring_buffer == NULL || data == NULL) return -1;

    power_ring_buffer_t *rb = handle->ring_buffer;

    count = (len < rb->current_count) ? len : rb->current_count;

    for (i = 0; i < count; i++) {
        data[i] = rb->current_buffer[rb->current_read_idx];
        rb->current_read_idx = (rb->current_read_idx + 1) % POWER_DATA_BUFFER_SIZE;
        rb->current_count--;
    }

    if (read_count) *read_count = count;

    return 0;
}

/** 清空缓冲区 **/
void power_buffer_clear(power_handle_t *handle)
{
    if (handle == NULL || handle->ring_buffer == NULL) return;
    memset(handle->ring_buffer, 0, sizeof(power_ring_buffer_t));
}
