/**
 * app_pos.c - Position sensor application layer.
 *
 * app_pos_init()  → module init + start
 * app_pos_task()  → read both M1/M2, update global data
 */

#include "app_pos.h"
#include "pos_module.h"
#include "msg_pubsub.h"
#include "msg_topics.h"
#include "sys_config.h"
#include "system.h"

#if SYS_ENABLE_POS

extern const pos_hw_cfg_t POS_HW_CFG_M1;
extern const pos_hw_cfg_t POS_HW_CFG_M2;

pos_handle_t pos_m1;
pos_handle_t pos_m2;

static const pos_sw_cfg_t POS_SW_CFG_M1 = {
    .pos_res = 33,
};

static const pos_sw_cfg_t POS_SW_CFG_M2 = {
    .pos_res = 33,
};

static const pos_cfg_t POS_CFG_M1 = {
    .hw = &POS_HW_CFG_M1,
    .sw = &POS_SW_CFG_M1,
};

static const pos_cfg_t POS_CFG_M2 = {
    .hw = &POS_HW_CFG_M2,
    .sw = &POS_SW_CFG_M2,
};

pos_upd_t s_pos_msg[POS_NUM] = {
    {
        .name = "POS1",
        .idx = 0,
        .cur_pos = 0,
        .status_pot = POT_NORMAL,
    },
    {
        .name = "POS2",
        .idx = 1,
        .cur_pos = 0,
        .status_pot = POT_NORMAL,
    },
};

/**
* @brief 处理缓存区数据（从缓冲区读取并滤波 -- 先以最简单的方法实现）
 */
static void process_single_channel(pos_handle_t *handle)
{
    float voltage_buffer[40];
    uint16_t read_count;

    if (pos_buffer_read(handle, voltage_buffer, sizeof(voltage_buffer), &read_count) != 0 || read_count == 0)
        return;

    float sum = 0;
    for (uint8_t i = 0; i < read_count; i++) {
        sum += voltage_buffer[i];
    }
    handle->pos_upd.cur_pos = sum / read_count;
}

static void app_process_pot_data(void)
{
    process_single_channel(&pos_m1);
    process_single_channel(&pos_m2);
}

/**
* @brief 异常检测
 */
static void app_pos_check(void) //异常标准待确认
{
	if (ACT_DIR_COMBINE_IS_MOVE_IN == sys_get_dir(0))
	{
			//结合 = 缩回机型
		if (pos_m1.pos_upd.cur_pos > 3.25f)
		{
			pos_m1.pos_upd.status_pot = POS_NO;
		}
		else if (pos_m1.pos_upd.cur_pos < 1.25f)
		{
			pos_m1.pos_upd.status_pot = POS_ERR;
		}
		else
		{
			pos_m1.pos_upd.status_pot = POT_NORMAL;
		}
	}else
	{
		if (pos_m1.pos_upd.cur_pos > 3.25f)
		{
			pos_m1.pos_upd.status_pot = POS_NO;
		}
		else if (pos_m1.pos_upd.cur_pos < 0.25f)
		{
			pos_m1.pos_upd.status_pot = POS_ERR;
		}
		else
		{
			pos_m1.pos_upd.status_pot = POT_NORMAL;
		}
	}

	if (ACT_DIR_COMBINE_IS_MOVE_IN == sys_get_dir(1))
	{
		//结合 = 缩回机型
		if (pos_m2.pos_upd.cur_pos > 3.25f)
		{
			pos_m2.pos_upd.status_pot = POS_NO;
		}
		else if (pos_m2.pos_upd.cur_pos < 1.25f)
		{
			pos_m2.pos_upd.status_pot = POS_ERR;
		}
		else
		{
			pos_m2.pos_upd.status_pot = POT_NORMAL;
		}
	}else
	{
		if (pos_m2.pos_upd.cur_pos > 3.25f)
		{
			pos_m2.pos_upd.status_pot = POS_NO;
		}
		else if (pos_m2.pos_upd.cur_pos < 0.25f)
		{
			pos_m2.pos_upd.status_pot = POS_ERR;
		}
		else
		{
			pos_m2.pos_upd.status_pot = POT_NORMAL;
		}
	}

}

/**
* @brief 状态更新
 */
static void app_pos_upd(void)
{
	s_pos_msg[POS_M1].cur_pos = pos_m1.pos_upd.cur_pos;
	s_pos_msg[POS_M1].status_pot = pos_m1.pos_upd.status_pot;

	s_pos_msg[POS_M2].cur_pos = pos_m2.pos_upd.cur_pos;
	s_pos_msg[POS_M2].status_pot = pos_m2.pos_upd.status_pot;
}

void app_pos_data_process(void)//放adc中断
{
	pos_module_process(&pos_m1);
	pos_module_process(&pos_m2);
}

void app_pos_init(void)
{
	pos_module_init(&pos_m1, &POS_CFG_M1);
	pos_module_init(&pos_m2, &POS_CFG_M2);

	ADC_TriggerCmd(CM_ADC1, ADC_SEQ_B, ENABLE);
}

void app_pos_task(void)
{
	app_process_pot_data();
	app_pos_check();
	app_pos_upd();

	//状态发布
	Msg_Publish(TOPIC_POS_UPDATED, &s_pos_msg[POS_M1], sizeof(pos_upd_t), MSG_PRIO_HIGH);
	Msg_Publish(TOPIC_POS_UPDATED, &s_pos_msg[POS_M2], sizeof(pos_upd_t), MSG_PRIO_HIGH);
}

#endif
