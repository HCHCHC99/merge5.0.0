#include <stdint.h>
#include <string.h>
#include "work_config.h"    // 必须包含配置头文件
#include "axis_typedef.h"
#include "flash_mcu.h"

/* 全局系统对象 */
extern System_t mySystem;

/**
 * @brief 根据车型ID+逻辑标记查找配置表
 * @param car_id 车型序号
 * @param logic_flag 搭配模式标记
 * @param table_idx 输出匹配到的表索引
 * @return FIND_SUCCESS 匹配成功 / FIND_FAIL 无匹配
 */
FindRet_t WorkConfig_Find(uint8_t car_id, uint8_t logic_flag, uint16_t *table_idx)
{
    if(table_idx == NULL)
    {
        return FIND_FAIL;
    }

    for (uint16_t i = 0; i < WORK_CONFIG_TABLE_CNT; i++)
    {
        const Work_Config_t *p_cfg = &WORK_CONFIG_TABLE[i];
        if(p_cfg->bits.car_id == car_id && p_cfg->bits.logic_flag == logic_flag)
        {
            *table_idx = i;
            return FIND_SUCCESS;
        }
    }
    return FIND_FAIL;
}

/**
 * @brief 系统初始化时加载推杆配置
 * @note 在存储模块、系统状态机初始化完成后调用
 */
void work_config_init(void){

    // 获取存储模块完整机型配置结构体（替代原来读取单uint16数值）
    Work_Config_t store_cfg = bsp_read_model();
    uint8_t car_id = store_cfg.bits.car_id;
    uint8_t cur_logic_flag = store_cfg.bits.logic_flag;
    uint16_t table_idx = 0;
    FindRet_t find_ret;

    // 1. 校验车型号范围，超限强制使用0号车型
    if(car_id >= W_MODEL_NUM_MAX)
    {
        car_id = 0;
        // 此处可加日志：车型号超出范围，默认使用0号车型
    }

	mySystem.eff_model = car_id;

    // 2. 使用读取到的car_id、logic_flag查表匹配推杆逻辑
    find_ret = WorkConfig_Find(car_id, cur_logic_flag, &table_idx);

    if(find_ret == FIND_SUCCESS){

        const Work_Config_t *p_match = &WORK_CONFIG_TABLE[table_idx];

        for(uint8_t axis_no = 0; axis_no < W_AXIS_NUM_MAX; axis_no++){

            Logic_t logic_val = (Logic_t)p_match->bits.axis[axis_no].val;

            switch(logic_val){
                case LOGIC_NONE:
                    mySystem.axis[axis_no].dir = ACT_DIR_NONE;
                    break;
                case LOGIC_A:
                    mySystem.axis[axis_no].dir = ACT_DIR_COMBINE_IS_MOVE_OUT;
                    break;
                case LOGIC_B:
                    mySystem.axis[axis_no].dir = ACT_DIR_COMBINE_IS_MOVE_IN;
                    break;
                default:
                    mySystem.axis[axis_no].dir = ACT_DIR_NONE;
                    // 日志：无效推杆逻辑值
                    break;
                }
            }
    }
    else
    {
		mySystem.axis[0].dir = ACT_DIR_COMBINE_IS_MOVE_OUT;
		mySystem.axis[1].dir = ACT_DIR_COMBINE_IS_MOVE_IN;
		bsp_write_model(WORK_CONFIG_TABLE[17]);//默认CF机型
        // 日志：当前车型无匹配配置，使用默认无推杆
    }

	//读结合分离点 -- 26.7.21zjw
	 for(uint8_t axis_no = 0; axis_no < W_AXIS_NUM_MAX; axis_no++){

		 if (true == bsp_read_cali(axis_no))//没存标定点
		 {
			 mySystem.axis[axis_no].is_calib = true;
			 mySystem.axis[axis_no].pos_separate = bsp_read_pos_separate(axis_no);
			 mySystem.axis[axis_no].pos_combine = bsp_read_pos_combine(axis_no);
		 }

	 }
}

/**
 * @brief 系统工作模式更改
 * @note 系统运行中，成功标定，那么更改系统工作模式，并存储
 */
void work_config_modify(uint8_t car_id, uint8_t logic_flag)
{
    uint16_t table_idx = 0;
    FindRet_t find_ret;

    find_ret = WorkConfig_Find(car_id, logic_flag, &table_idx);

    if(find_ret == FIND_SUCCESS){

    // 使用宏构造完整配置结构体，直接写入Flash
    Work_Config_t new_cfg = WORK_CONFIG_TABLE[table_idx];
    bsp_write_model(new_cfg);

    // 重新加载更新系统推杆方向
    work_config_init();
	}
}
