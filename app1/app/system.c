/**
 * @file    system.c
 * @brief   控制器顶层系统实现
 */
#include "system.h"
#include "sys_state.h"
#include "act_state.h"
#include "mot_state.h"
#include "axis_typedef.h"
#include "msg_pubsub.h"
#include "msg_topics.h"
#include <stddef.h>
#include "sys_tick.h"

#include "power_module.h"
#include "app_can.h"
#include "can_module.h"
#include "key.h"
#include "ring_buffer.h"
#include "flash_mcu.h"
#include "led.h"
#include "key_state.h"

// 获取数组元素个数
#define ARRAY_ELEM_COUNT(arr)      (sizeof(arr) / sizeof((arr)[0]))
// 获取数组单元素字节大小
#define ARRAY_ELEM_SIZE(arr)       (sizeof((arr)[0]))

/* 全局系统对象 */
System_t mySystem;

// 环形缓冲区数组
#define RING_BUFFER_SIZE            (50)
float axis_1_speed_buff[RING_BUFFER_SIZE];     // 速度采样 环形缓冲区
float axis_2_speed_buff[RING_BUFFER_SIZE];     // 速度采样 环形缓冲区

//260713_add:
#define POS_BASE_MM           (7.82f)
#define POS_BASE_ADC          (0.5f)

const char *event_group_system_names[][2] = {
    {"system_event","system_event_"},
    {"event_act_1","event_act_2"},
    {"event_mot_1","event_mot_2"},
    {"event_can_1","event_can_2"},
    {"event_calib_1","event_calib_2"},
    {"event_key","event_key_"},
};

typedef struct {
    char                name[4];
    uint8_t             idx;
    float               cur_pos;        // 当前位置（绝对位置）
    pot_stat_t        status_pot;    // 传感器状态
} S_pos_upd_t;

/**
 * @brief 系统状态 - 位置信息回调处理函数
 *
 * @param topic  消息主题ID（用于区分不同类型的消息）
 * @param data   指向位置信息数据缓冲区的指针
 * @param len    数据缓冲区长度（单位：字节）
 * @param prio   消息优先级
 *
 * @note 该函数用于接收并处理系统发布的位置状态数据
 */
void System_Pos_Callback(uint32_t topic, const void* data, uint16_t len, uint8_t prio){
    // 安全校验：数据长度必须匹配结构体长度
    if (data == NULL || len != sizeof(S_pos_upd_t)) {
        return; // 数据无效直接退出
    }

    // 把 data 转换成结构体指针
    const S_pos_upd_t* p_data = (const S_pos_upd_t*)data;

	mySystem.axis[p_data->idx].pos_err = p_data->status_pot;

    if (p_data->idx < MAX_AXIS_NUM  &&  p_data->status_pot != POS_NO )
    {

        // 缓冲区满，计算一次推杆速度
        if ( mySystem.axis[p_data->idx].speed_data->write(mySystem.axis[p_data->idx].speed_data,&p_data->cur_pos) == RING_BUF_ERR_FULL )
        {
            uint32_t now = SysTick_GetTick();

            // 计算 当前位置，原始 adc采样电压
            float floatSum = mySystem.axis[p_data->idx].speed_data->getSumFloat(mySystem.axis[p_data->idx].speed_data);
            mySystem.axis[p_data->idx].pos_current = floatSum / (RING_BUFFER_SIZE-1 ) ;

            mySystem.axis[p_data->idx].pos_ctrl.fdb_last = mySystem.axis[p_data->idx].pos_ctrl.fdb;
            mySystem.axis[p_data->idx].pos_ctrl.fdb_last_time = mySystem.axis[p_data->idx].pos_ctrl.fdb_time;

            uint16_t temp = (uint16_t)((mySystem.axis[p_data->idx].pos_current * 33.3f)*10.0f);   // 当前位置  单位：mm，最大值 约50.0000*10
            mySystem.axis[p_data->idx].pos_current = (float)temp / 10.0f;
           // mySystem.axis[p_data->idx].pos_current = (mySystem.axis[p_data->idx].pos_current - POS_BASE_ADC)* 30.3f + POS_BASE_MM;           // 保留 小数点后一位

            mySystem.axis[p_data->idx].pos_ctrl.fdb = mySystem.axis[p_data->idx].pos_current;
            mySystem.axis[p_data->idx].pos_ctrl.fdb_time = now;

            if (mySystem.axis[p_data->idx].pos_ctrl.fdb < mySystem.axis[p_data->idx].pos_combine + mySystem.axis[p_data->idx].pos_ctrl.stall_thr &&
                mySystem.axis[p_data->idx].pos_ctrl.fdb > mySystem.axis[p_data->idx].pos_combine - mySystem.axis[p_data->idx].pos_ctrl.stall_thr )
            {
                mySystem.axis[p_data->idx].position = ACT_POS_COMBINE;
            }else if (mySystem.axis[p_data->idx].pos_ctrl.fdb < mySystem.axis[p_data->idx].pos_separate + mySystem.axis[p_data->idx].pos_ctrl.stall_thr &&
                    mySystem.axis[p_data->idx].pos_ctrl.fdb > mySystem.axis[p_data->idx].pos_separate - mySystem.axis[p_data->idx].pos_ctrl.stall_thr )
            {
                mySystem.axis[p_data->idx].position = ACT_POS_SEPARATE;
            }else{
                mySystem.axis[p_data->idx].position = ACT_POS_MIDDLE;
            }

//            /*260708——速度环取消，屏蔽掉计算*/
//            // 速度单位：mm/s
//            // 计算当前速度
//            //260702：增加，需要扩展速度计算窗口，加上时间判断超过固定时间后才计算;now 返回毫秒
//            if((now-mySystem.axis[p_data->idx].speed.timeout) >= 400)    //260706_待测试
//            {
//                mySystem.axis[p_data->idx].speed.speed_value = (mySystem.axis[p_data->idx].pos_current - mySystem.axis[p_data->idx].speed.pos) / ((float)(now-mySystem.axis[p_data->idx].speed.timeout)/ 1000) ;
//                mySystem.axis[p_data->idx].speed.pos = mySystem.axis[p_data->idx].pos_current;
//                mySystem.axis[p_data->idx].speed.timeout = now;
//            }

//            // 判定速度是否 为负值
//            if (mySystem.axis[p_data->idx].speed.speed_value < 0 )
//            {
//                mySystem.axis[p_data->idx].speed.speed_value *= -1;
//            }

//            // 判定速度是否超范围  核算 推杆速  通过计算 推杆 全速约 6mm/s,设定阈值 ，超阈值 丢弃
//            if (mySystem.axis[p_data->idx].speed.speed_value < mySystem.axis[p_data->idx].speed.speed_limit)
//            {
//                // 保留 速度小数点后 1 位
//                mySystem.axis[p_data->idx].speed.speed_value = (float)((uint16_t)(mySystem.axis[p_data->idx].speed.speed_value * 100 ))/ 100.0f ;
//                mySystem.axis[p_data->idx].spd_ctrl.fdb      = mySystem.axis[p_data->idx].speed.speed_value;
//            }
            mySystem.axis[p_data->idx].speed_data->clear(mySystem.axis[p_data->idx].speed_data);    //260622_RL_add:扈工提供修改方案-清空环形缓存区
        }

    }

	mySystem.axis[p_data->idx].pos_vol = (uint16_t)(p_data->cur_pos * 1000);
}

/**
 * @brief 系统状态 - 电源电压  回调处理函数
 *
 * @param topic  消息主题ID（用于区分不同类型的消息）
 * @param data   指向位置信息数据缓冲区的指针
 * @param len    数据缓冲区长度（单位：字节）
 * @param prio   消息优先级
 *
 * @note 该函数用于接收并处理系统发布的电源状态数据
 */
void System_Power_Callback(uint32_t topic, const void* data, uint16_t len, uint8_t prio){
    // 安全校验：数据长度必须匹配结构体长度
    if (data == NULL || len != sizeof(power_upd_t)) {
        return; // 数据无效直接退出
    }

	//如果系统不处于工作状态，直接返回
	if (mySystem.sys_sm.cur_state == SYS_STATE_INIT)
	{
		return;
    }

    // 把 data 转换成结构体指针
    const power_upd_t* p_data = (const power_upd_t*)data;

    mySystem.voltage = p_data->voltage;

    // 目前只处理 电源电压状态
    if (p_data->idx == 0 ){
		if (false == mySystem.acc_enable)
		{
			EventGroup_Send(mySystem.sys_evt, EVT_SYS_FAULT);
		}else
		{
            if (p_data->status_volt == POWER_STATUS_OVER_VOLT){
                EventGroup_Send(mySystem.sys_evt,EVT_SYS_VOLT_OVER);
            }else if (p_data->status_volt == POWER_STATUS_UNDER_VOLT){
				EventGroup_Send(mySystem.sys_evt, EVT_SYS_VOLT_UNDER);
			}else
			{
				EventGroup_Send(mySystem.sys_evt, EVT_SYS_RECOVERY | EVT_SYS_CMD_WORK_ENABLE);//系统故障恢复
			}
		}
	}

	//更新电流
	if (p_data->idx == 1 || p_data->idx == 2)
	{
		mySystem.axis[p_data->idx - 1].current = p_data->current;
	}

}

/**
 * @brief CAN 消息接收
 *
 * @param topic  消息主题ID（用于区分不同类型的消息）
 * @param data   指向位置信息数据缓冲区的指针
 * @param len    数据缓冲区长度（单位：字节）
 * @param prio   消息优先级
 *
 * @note 该函数用于接收并处理系统发布的 can数据
 */
void System_CAN_Callback(uint32_t topic, const void* data, uint16_t len, uint8_t prio){

// 安全校验：数据长度必须匹配结构体长度
    if (data == NULL || len != sizeof(can_msg_t)) {
        return; // 数据无效直接退出
    }
	//如果系统不处于工作状态，直接返回
	if (mySystem.sys_sm.cur_state != SYS_STATE_RUN)
	{
		return;
	}
    // 把 data 转换成结构体指针
    const can_msg_t* p_data = (const can_msg_t*)data;

	//手动标定

	//报文标定 -- 不支持一键自动标定
	if (AWD_CAL_ENABLE == p_data->awd_cal_enable)
	{
		can_clr_awd_cal();
		key_state_reset();

		if (false == mySystem.first_calib)
		{
			if (CALIB_NOT_SUPPORT == p_data->support_calibration)//不支持一键自动标定
		    {

				//判定标定状态机状态，如果处于idle,说明没在手动标定，直接记当前位置就行
				for (uint8_t j = 0; j < MOT_INDEX_MAX; j++)
				{
					if (ACT_MODE_NORMAL == mySystem.axis[j].mode || ACT_MODE_IDLE == mySystem.axis[j].mode)
					{
						mySystem.first_calib = true;//第一次标定的标志：无论标定成功失败，一次上电都只允许标定一次
						model_pos_upd(j);

//						if (mySystem.axis[j].dir == ACT_DIR_COMBINE_IS_MOVE_OUT)
//						{
//							//存储当前标定位置+已标定标记+机型
//							upd_model();
//							bsp_write_cali(j, 1);
//							bsp_write_pos_separate(j, mySystem.axis[j].pos_current);
//							work_config_init();
//
//						}else if (mySystem.axis[j].dir == ACT_DIR_COMBINE_IS_MOVE_IN)
//						{
//							if (mySystem.axis[j].pos_current > (16 + 22 + 1))
//							{
//								//存储当前标定位置+已标定标记+机型
//								upd_model();
//								bsp_write_cali(j, 1);
//								bsp_write_pos_separate(j, mySystem.axis[j].pos_current);
//								work_config_init();
//							}else//位置不满足要求
//							{
//								//不保存标定位置，但是要保存机型
//							}
//						}
					}else
					{
						mySystem.first_calib = true;
						EventGroup_Send(mySystem.axis[0].evt_calib, EVT_CALIB_DONE);
						EventGroup_Send(mySystem.axis[1].evt_calib, EVT_CALIB_DONE);
					}
				}
		    }
		    else if (CALIB_SUPPORT == p_data->support_calibration)//一键自动标定之前要根据机型调整运行方向
		    {
				mySystem.first_calib = true;
				if (mySystem.model)
				{
					bool cur_logic_flag = false;
					cur_logic_flag = spc_model_get(mySystem.model);
					work_config_modify(mySystem.model, cur_logic_flag);
				}

				for (int i = 0; i < MAX_AXIS_NUM; i++) {
					if (ACT_DIR_NONE != mySystem.axis[i].dir && POS_NO != mySystem.axis[i].pos_err)
					{
						mySystem.axis[i].is_calib = true;
						EventGroup_Send(mySystem.axis[i].evt_calib, EVT_CALIB_AUTO_CALIB_SEP);
					}
				}

		    }
		}
		else//如果非第一次上电，如果处于手动标定状态，需要退出手动标定状态并且不保存
		{
			EventGroup_Send(mySystem.axis[0].evt_calib, EVT_CALIB_EXIT);
			EventGroup_Send(mySystem.axis[1].evt_calib, EVT_CALIB_EXIT);
		}
	}

	//最高档信号
	if (true == p_data->highest_gear_signal || GEAR_4 == p_data->gear_status)
	{
		mySystem.can_over_gear_enable = true;
//
//		EventGroup_Send(mySystem.axis[0].evt_act, EVT_ACT_HGEAR_NO_COMB);
//		EventGroup_Send(mySystem.axis[1].evt_act, EVT_ACT_HGEAR_NO_COMB);
	}else//can中的最高档信号取消
	{
		mySystem.can_over_gear_enable = false;
//		if (false == mySystem.key_over_gear_enable)//按键中的也取消
//		{
//			//向推杆状态机发送退出高档位事件
//			EventGroup_Send(mySystem.axis[0].evt_act, EVT_ACT_HGEAR_CANNEL);
//			EventGroup_Send(mySystem.axis[1].evt_act, EVT_ACT_HGEAR_CANNEL);
//		}
	}

    //超速
    if (p_data->speed_general > 15 || p_data->speed_shengshuo > 15)
	{
		mySystem.over_spd_enable = true;
//		//向推杆状态机发送超速事件
//		EventGroup_Send(mySystem.axis[0].evt_act, EVT_ACT_OVERSPEED_ENABLE);
//		EventGroup_Send(mySystem.axis[1].evt_act, EVT_ACT_OVERSPEED_ENABLE);

	}
	else
	{
		mySystem.over_spd_enable = false;

//		//向推杆发送取消超速事件
//		EventGroup_Send(mySystem.axis[0].evt_act, EVT_ACT_OVERSPEED_DISABLE);
//		EventGroup_Send(mySystem.axis[1].evt_act, EVT_ACT_OVERSPEED_DISABLE);
	}

	//结合 -- M1
	if (CAN_CMD_AUTO_COMB == p_data->motor1_cmd)
	{
		can_clr_cmd(0);
        if (ACT_MODE_CALIB_MODE == mySystem.axis[0].mode)
		{
			key_state_reset();
			EventGroup_Send(mySystem.axis[0].evt_calib, EVT_CALIB_AUTO_COMB);//可以结合--发给推杆状态机
		}
		else
		{
			EventGroup_Send(mySystem.axis[0].evt_act, EVT_ACT_COMBINE);//可以结合--发给推杆状态机
		}
	} else if(CAN_CMD_AUTO_SEP == p_data->motor1_cmd)
    {
		can_clr_cmd(0);
        if (ACT_MODE_CALIB_MODE == mySystem.axis[0].mode)
		{
			key_state_reset();
			EventGroup_Send(mySystem.axis[0].evt_calib, EVT_CALIB_AUTO_SEP);//可以结合--发给推杆状态机
		}
		else
		{
			EventGroup_Send(mySystem.axis[0].evt_act, EVT_ACT_SEPARATE);//可以结合--发给推杆状态机
		}
    }

    if (CAN_CMD_AUTO_COMB == p_data->motor2_cmd)
    {
		can_clr_cmd(1);
		if (ACT_MODE_CALIB_MODE == mySystem.axis[1].mode)
		{
			key_state_reset();
			EventGroup_Send(mySystem.axis[1].evt_calib, EVT_CALIB_AUTO_COMB);//可以结合--发给推杆状态机
		}
		else
		{
			EventGroup_Send(mySystem.axis[1].evt_act, EVT_ACT_COMBINE);//可以结合--发给推杆状态机
		}
	} else if(CAN_CMD_AUTO_SEP == p_data->motor2_cmd)
    {
		can_clr_cmd(1);
		if (ACT_MODE_CALIB_MODE == mySystem.axis[1].mode)
		{
			key_state_reset();
			EventGroup_Send(mySystem.axis[1].evt_calib, EVT_CALIB_AUTO_SEP);//可以结合--发给推杆状态机
		}
		else
		{
			EventGroup_Send(mySystem.axis[1].evt_act, EVT_ACT_SEPARATE);//可以结合--发给推杆状态机
		}
	}

	//机型更新
	if (p_data->machine_type != 0)
	{
		mySystem.model = p_data->machine_type;
	}

	//特殊机型 -- 干什么
	if (true == p_data->ce4p_spc)
	{
		mySystem.model_ce_1 = true;
	}else
	{
		mySystem.model_ce_1 = false;
	}

	if (true == p_data->en_gk_only)
	{
		mySystem.model_gk_1 = true;
	}else
	{
		mySystem.model_gk_1 = false;
	}
}

/**
 * @brief key 状态识别
 *
 * @param topic  消息主题ID（用于区分不同类型的消息）
 * @param data   指向位置信息数据缓冲区的指针
 * @param len    数据缓冲区长度（单位：字节）
 * @param prio   消息优先级
 *
 * @note 该函数用于接收并处理系统发布的 key 状态
 */
void System_Key_Callback(uint32_t topic, const void* data, uint16_t len, uint8_t prio){
    // 安全校验：数据长度必须匹配结构体长度
    if (data == NULL || len != sizeof(Key_Report_t)) {
        return; // 数据无效直接退出
    }

	//如果系统不处于工作状态，直接返回
	if (mySystem.sys_sm.cur_state == SYS_STATE_INIT){
		return;
	}
    // 把 data 转换成结构体指针
    const Key_Report_t* p_data = (const Key_Report_t*)data;
	static uint32_t trig_time = 0;
	static bool trig_flg = false;

    // ACC
	if (0x02 == p_data->key_code){		//稳压源限流3A以下&电流环4000时会因为电压拉低误触发acc失能，以后遇到了要注意一下
			//acc标志
			if (KEY_EVENT_LONG == p_data->event){
					mySystem.acc_enable = true;
					mySystem.axis[0].acc_enable =  mySystem.acc_enable;
					mySystem.axis[1].acc_enable =  mySystem.acc_enable;
			}else{
					mySystem.acc_enable = false;
					mySystem.axis[0].acc_enable =  mySystem.acc_enable;
					mySystem.axis[1].acc_enable =  mySystem.acc_enable;
			}
	}

	//高档位
	if (0x05 == p_data->key_code){
        if (KEY_EVENT_LONG == p_data->event){                 //有效
			mySystem.key_over_gear_enable = true;
			EventGroup_Send(mySystem.axis[0].evt_act, EVT_ACT_HGEAR_NO_COMB);
			EventGroup_Send(mySystem.axis[1].evt_act, EVT_ACT_HGEAR_NO_COMB);
		}else{
			mySystem.key_over_gear_enable = false;
			//向推杆状态机发送退出高档位事件
			if (false == mySystem.can_over_gear_enable)
			{
				EventGroup_Send(mySystem.axis[0].evt_act, EVT_ACT_HGEAR_CANNEL);
				EventGroup_Send(mySystem.axis[1].evt_act, EVT_ACT_HGEAR_CANNEL);
			}

		}
	}

	//结合 JH_KEY 外部整机 结合信号
	if (0x03 == p_data->key_code){
		if (KEY_EVENT_CLICK == p_data->event || KEY_EVENT_LONG == p_data->event)
		{
			key_state_reset();
			if (ACT_MODE_CALIB_MODE == mySystem.axis[0].mode)
			{
				EventGroup_Send(mySystem.axis[0].evt_calib, EVT_CALIB_AUTO_COMB);//可以结合--发给推杆状态机
			}
			else
			{
				EventGroup_Send(mySystem.axis[0].evt_act, EVT_ACT_COMBINE);//可以结合--发给推杆状态机
			}

			if (ACT_MODE_CALIB_MODE == mySystem.axis[1].mode)
			{
				EventGroup_Send(mySystem.axis[1].evt_calib, EVT_CALIB_AUTO_COMB);//可以结合--发给推杆状态机
			}
			else
			{
				EventGroup_Send(mySystem.axis[1].evt_act, EVT_ACT_COMBINE);//可以结合--发给推杆状态机
			}
		}

	}

	//分离 FL_KEY 外部整机 分离信号
	if (0x04 == p_data->key_code){
		if (KEY_EVENT_CLICK == p_data->event || KEY_EVENT_LONG == p_data->event)
		{
			key_state_reset();
			if (ACT_MODE_CALIB_MODE == mySystem.axis[0].mode)
			{
				EventGroup_Send(mySystem.axis[0].evt_calib, EVT_CALIB_AUTO_SEP);//可以结合--发给推杆状态机
			}
			else
			{
				EventGroup_Send(mySystem.axis[0].evt_act, EVT_ACT_SEPARATE);//可以结合--发给推杆状态机
			}

			if (ACT_MODE_CALIB_MODE == mySystem.axis[1].mode)
			{
				EventGroup_Send(mySystem.axis[1].evt_calib, EVT_CALIB_AUTO_SEP);//可以结合--发给推杆状态机
			}
			else
			{
				EventGroup_Send(mySystem.axis[1].evt_act, EVT_ACT_SEPARATE);//可以结合--发给推杆状态机
			}
		}

	}
}

AxisDir_t sys_get_dir(uint8_t idx)
{
	return mySystem.axis[idx].dir;
}

/**
 * @brief  系统初始化
 * @note   状态机、事件组
 */
void State_Init(void)
{
    // 系统状态机 & 事件组初始化
    Sys_State_Init(&mySystem.sys_sm);
    mySystem.sys_evt = EventGroup_Create(event_group_system_names[0][0]);

    // GPIO 初始化
    stc_gpio_init_t stcGpioInit;
    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PinDir = PIN_DIR_OUT;

    RingBuf_CreateInstance("Vaxis_1", axis_1_speed_buff, ARRAY_ELEM_COUNT(axis_1_speed_buff), ARRAY_ELEM_SIZE(axis_1_speed_buff), RING_MODE_DISCARD);
    RingBuf_CreateInstance("Vaxis_2", axis_2_speed_buff, ARRAY_ELEM_COUNT(axis_2_speed_buff), ARRAY_ELEM_SIZE(axis_2_speed_buff), RING_MODE_DISCARD);

    // 推杆 & 电机初始化
    for (int i = 0; i < MAX_AXIS_NUM; i++) {
        Axis_t *ax = &mySystem.axis[i];
        ax->id = i;
        ax->enable = 0;
        ax->stall_count = 0;
        ax->stall_count_thr = 7500;     //260630——fix:堵转检测时间200us*N = Xms。
        ax->success_count = 0;
        ax->success_count_thr =0;    //260630——fix:到位置检测时间200us* N = 10ms。
        ax->startup_mask_thr = 500;    //260625_Rl_add:启动堵转检测屏蔽时间

        // 测试时，写死，实际应从 标定数据获取

        //260718_add:暂定上下断电点 位置:为目前推杆采样计算得到的位置69和20
        ax->pos_top_limit = 85.0f;
        ax->pos_down_limit = 35.0f;
        ax->limit_count_thr = 100;

        if (i == 0)
        {
            ax->speed_data = RingBuf_GetByName("Vaxis_1");
        }else if (i == 1)
        {
            ax->speed_data = RingBuf_GetByName("Vaxis_2");
        }

        ax->speed.speed_value = 0.0;
        ax->speed.pos = 0.0;
        ax->speed.speed_limit = 8.0;
        ax->speed.timeout_thr = 20;

        ax->evt_act = EventGroup_Create(event_group_system_names[1][i]);
        Act_State_Init(&ax->sm_act);

        ax->motor.id = i;
        ax->motor.enable = 0;
        ax->motor.evt_mot = EventGroup_Create(event_group_system_names[2][i]);
        Mot_State_Init(&ax->motor.sm_mot);

        Motor_t *mot = &mySystem.axis[i].motor;
        mot->hw = &MOTOR_HW_LIST[i];
        if (mot->hw->hu->active_level) {
            stcGpioInit.u16PullUp = PIN_STAT_RST;
        } else {
            stcGpioInit.u16PullUp = PIN_STAT_SET;
        }
        GPIO_Init(mot->hw->hu->gpio_port, mot->hw->hu->pin, &stcGpioInit);

        if (mot->hw->hv->active_level) {
            stcGpioInit.u16PullUp = PIN_STAT_RST;
        } else {
            stcGpioInit.u16PullUp = PIN_STAT_SET;
        }
        GPIO_Init(mot->hw->hv->gpio_port, mot->hw->hv->pin, &stcGpioInit);

		ax->evt_calib = EventGroup_Create(event_group_system_names[4][i]);
        Calib_State_Init(&ax->sm_calib);

    }

    // 需要 订阅 数据
    // 1.位置信息
    // 2.速度信息(通过位置信息计算)
    Msg_Subscribe(TOPIC_POS_UPDATED, System_Pos_Callback);
    // 3.电源电压信息
    Msg_Subscribe(TOPIC_PWR_STATE_UPDATED, System_Power_Callback);
    // 4.can指令数据
    Msg_Subscribe(TOPIC_CAN_MSG_RECEIVED, System_CAN_Callback);
//    // 5.外部IO数据，抽象成 按键指令数据
    Msg_Subscribe(TOPIC_KEYS_STATE, System_Key_Callback);

    //系统初始化完成
    EventGroup_Send(mySystem.sys_evt, EVT_SYS_INIT_DONE | EVT_SYS_CMD_WORK_ENABLE);
}

/**
 * @brief  系统周期调度任务
 * @note   依次调度系统、推杆、电机状态机
 */
void State_Task(void)
{
    // 调度系统状态 任务
    Sys_State_Task();

    // 调度所有轴状态 任务
	// 进入标定状态有两种方式：can和key。key需要在正常模式，调度标定状态机为前提，因为key向标定状态机发送事件
    for (int i = 0; i < MAX_AXIS_NUM; i++) {
		mySystem.target_id = i;
		Calib_State_Task(&mySystem.axis[i]);
        Act_State_Task(&mySystem.axis[i]);
        Mot_State_Task(&mySystem.axis[i]);
//        Can_State_Task(&mySystem.axis[i]);

    }

//    // 调度所有轴状态 任务
//    for (int i = 0; i < MAX_AXIS_NUM; i++) {
//		if(mySystem.sys_sm.cur_state == SYS_STATE_IDLE ||
//			mySystem.sys_sm.cur_state == SYS_STATE_RUN){
//			Act_State_Task(&mySystem.axis[i]);
//		}else if(mySystem.sys_sm.cur_state == SYS_MODE_CALIB_MODE_1 ||
//					mySystem.sys_sm.cur_state == SYS_MODE_CALIB_MODE_2){
//			Calib_State_Task(&mySystem.axis[i]);
//		}
//		Mot_State_Task(&mySystem.axis[i]);
//    }
}
