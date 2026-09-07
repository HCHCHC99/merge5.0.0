/**
 * @file    act_state.h
 * @brief   推杆状态机接口
 */
#ifndef __ACT_STATE_H
#define __ACT_STATE_H

#include "axis_typedef.h"

/**
 * @brief 推杆状态枚举
 */
     //260701:
    //增加超速状态:静止时+超速信号——进入
    //超速报警状态：超速状态+分离/结合指令；分离/结合运行中+超速信号——进入
    //退出，退到idle

    //增加高档位状态:静止时+超速信号——进入。（允许分离）
    //高档位报警状态：超速状态+分离/结合指令；结合运行中+超速信号——进入。
    //退出，退到idle

    //执行结合/分离动作之前，判断位置是否在结合、分离点，在的话不执行动作。

    //自动标定：只有分离，遇阻回退2mm，完成。

    //重新构造标定的状态机，和现在推杆状态机并行

    //未标定模式、传感器异常时，推杆不能动（不能进入normal状态）
    //传感器异常：控制LED橘色常亮；未标定：红绿交替闪烁

typedef enum {
    ACT_STATE_INIT = 0,                   // 初始化状态
    ACT_STATE_IDLE ,                      // 空闲状态

    ACT_STATE_COMBINE = 2,
    ACT_STATE_COMBINE_TRY_1,
    ACT_STATE_COMBINE_TRY_2 = 4,            //try2时，成功就ok；失败的话，在回退1.2
//    ACT_STATE_COMBINE_TRY_3 = 5,      //多了一个，删除——260707
    ACT_STATE_COMBINE_BACK = 5,
    ACT_STATE_COMBINE_BACK_1,
    ACT_STATE_COMBINE_BACK_2,
    ACT_STATE_COMBINE_SUCCESS,
    ACT_STATE_COMBINE_FAIL = 9 ,

    ACT_STATE_SEPARATE = 10 ,
    ACT_STATE_SEPARATE_TRY_1,
    ACT_STATE_SEPARATE_TRY_2,
//    ACT_STATE_SEPARATE_TRY_3,     //删除——260707
    ACT_STATE_SEPARATE_BACK = 13 ,
    ACT_STATE_SEPARATE_BACK_1,
    ACT_STATE_SEPARATE_BACK_2,
    ACT_STATE_SEPARATE_SUCCESS ,
    ACT_STATE_SEPARATE_FAIL =17 ,

    ACT_STATE_DISABLE =18 ,
    ACT_STATE_HOLD ,                       // 保持/制动/停止
    ACT_STATE_ERROR ,                      // 错误

//    ACT_STATE_CALIBRATION = 22,             //260616_RL_add：标定        //260707—删除:推杆状态中不加入标定相关动作
//    ACT_STATE_CALIBRATION_OUT,              //260616_RL_add：标定伸出
//    ACT_STATE_CALIBRATION_IN = 24,          //260616_RL_add：标定伸出

    ACT_STATE_OVERSPEED = 21,                      //超速状态
    ACT_STATE_OVERSPEED_ALARM,                //超速报警状态
    ACT_STATE_H_GEAR_NO_COMB,                         //高档位状态
    ACT_STATE_H_GEAR_COMB_5S,                   //高档位报警状态
    ACT_STATE_H_GEAR_COMB_ED =25,                //结合点高档位报警状态

} ActuatorState_t;

//状态定义
typedef enum {
	CALIB_STATE_INIT = 0,
	CALIB_STATE_IDLE,
	CALIB_STATE_INTO_MANU_ING,
	CALIB_STATE_INTO_MANU_ED, // 手动标定 -- 已确认，但是按键未动手
	CALIB_STATE_MANU_IDLE,
	CALIB_STATE_MANU_OUT,
	CALIB_STATE_MANU_IN,
	CALIB_STATE_EXIT_MANU_ING,
	CALIB_STATE_AUTO_CALIB_SEP,
	CALIB_STATE_AUTO_CALIB_BACK,
}calib_state_t;

/**
 * @brief  推杆状态机初始化
 * @param  sm: 状态机指针
 * @param  evt: 事件组指针
 */
void Act_State_Init(StateMachine_t *sm);

/**
 * @brief  推杆状态机运行
 * @param  axis: 推杆对象指针
 */
void Act_State_Task(Axis_t *axis);

/*
 * //260606_RL_add:
 * @brief  推杆遇阻处理函数
 * @param  axis: 遇阻标志指针
 */
void act_babk_handler(void);

void Calib_State_Init(StateMachine_t *sm);
void Calib_State_Task(Axis_t *axis);

//特殊机型获取
bool spc_model_get(uint8_t model);
//机型配置
void model_pos_upd(uint8_t idx);

#endif
