/**
 * @file    mot_state.c
 * @brief   电机状态机实现
 * @note    仅负责电机驱动执行，无业务逻辑
 */
#include "mot_state.h"
#include "event_def.h"

/**
 * @brief 电机状态枚举（完全私有，对外不可见）
 */
typedef enum {
    MOT_STATE_IDLE = 0,                   // 空闲状态
    MOT_STATE_FORWARD,                    // 正转
    MOT_STATE_REVERSE,                    // 反转
    MOT_STATE_STOP,                       // 停止  自由停车
    MOT_STATE_BRAKE,                      // 制动
    MOT_STATE_ERROR,                      // 电机 错误
} MotorState_t;

/* 状态入口函数声明 */
static void mot_enter_idle(void);
static void mot_enter_run(void);
// static void mot_enter_forward(void);
// static void mot_enter_reverse(void);
// static void mot_enter_brake(void);
// static void mot_enter_stop(void);
// static void mot_enter_error(void);

/**
 * @brief 电机状态跳转表（全枚举定义）
 */
static const StateJumpTable_t mot_jump[] = {
    {MOT_STATE_IDLE,        EVT_MOT_FORWARD,      MOT_STATE_FORWARD},
    {MOT_STATE_IDLE,        EVT_MOT_REVERSE,      MOT_STATE_REVERSE},
    {MOT_STATE_IDLE,        EVT_MOT_ERROR,        MOT_STATE_ERROR},

    {MOT_STATE_FORWARD,     EVT_MOT_REVERSE,      MOT_STATE_REVERSE},
    {MOT_STATE_FORWARD,     EVT_MOT_STOP,         MOT_STATE_STOP},
    {MOT_STATE_FORWARD,     EVT_MOT_BRAKE,        MOT_STATE_BRAKE},
    {MOT_STATE_FORWARD,     EVT_MOT_ERROR,        MOT_STATE_ERROR},

    {MOT_STATE_REVERSE,     EVT_MOT_FORWARD,      MOT_STATE_FORWARD},
    {MOT_STATE_REVERSE,     EVT_MOT_STOP,         MOT_STATE_STOP},
    {MOT_STATE_REVERSE,     EVT_MOT_BRAKE,        MOT_STATE_BRAKE},
    {MOT_STATE_REVERSE,     EVT_MOT_ERROR,        MOT_STATE_ERROR},

    {MOT_STATE_STOP,        EVT_MOT_FORWARD,      MOT_STATE_FORWARD},
    {MOT_STATE_STOP,        EVT_MOT_REVERSE,      MOT_STATE_REVERSE},
    {MOT_STATE_STOP,        EVT_MOT_BRAKE,        MOT_STATE_BRAKE},
    {MOT_STATE_STOP,        EVT_MOT_ERROR,        MOT_STATE_ERROR},

    {MOT_STATE_BRAKE,       EVT_MOT_FORWARD,      MOT_STATE_FORWARD},
    {MOT_STATE_BRAKE,       EVT_MOT_REVERSE,      MOT_STATE_REVERSE},
    {MOT_STATE_BRAKE,       EVT_MOT_STOP,         MOT_STATE_STOP},
    {MOT_STATE_BRAKE,       EVT_MOT_ERROR,        MOT_STATE_ERROR},

    {MOT_STATE_ERROR,       EVT_MOT_RESET,        MOT_STATE_IDLE},
};

/**
 * @brief 电机状态入口函数表
 */
static const StateFuncTable_t mot_func[] = {
    {MOT_STATE_IDLE,        mot_enter_idle,    0},
    {MOT_STATE_FORWARD,     mot_enter_run,    0},
    {MOT_STATE_REVERSE,     mot_enter_run,    0},
    {MOT_STATE_BRAKE,       mot_enter_run,    0},
    {MOT_STATE_STOP,        mot_enter_run,    0},
    {MOT_STATE_ERROR,       mot_enter_run,    0},
};

/**
 * @brief 推杆状态机 初始化
 * @note
 */
void Mot_State_Init(StateMachine_t *sm)
{
    StateMachine_t *sm_temp = sm;
    sm_temp->jump_table = mot_jump;
    sm_temp->jump_table_size = sizeof(mot_jump)/sizeof(StateJumpTable_t);
    sm_temp->func_table = mot_func;
    sm_temp->func_table_size = sizeof(mot_func)/sizeof(StateFuncTable_t);
    sm_temp->init_state = MOT_STATE_IDLE;

    StateMachine_Init(sm_temp);
}

/**
 * @brief 电机状态机运行
 * @param  axis: 轴对象
 * @note  响应推杆指令，执行驱动状态流转
 */
void Mot_State_Task(Axis_t *axis)
{
    EventBits_t bits = EventGroup_Get(axis->motor.evt_mot);

	if (bits & EVT_MOT_STOP)
	{
		bits = EVT_MOT_STOP;
	}

//	if (bits & (EVT_MOT_STOP | EVT_MOT_REVERSE))
//	{
//		bits = EVT_MOT_REVERSE;
//	}
//
//	if (bits & (EVT_MOT_STOP | EVT_MOT_FORWARD))
//	{
//		bits = EVT_MOT_FORWARD;
//	}

    switch (bits)
    {
		case EVT_MOT_FORWARD:    // 正转
			StateMachine_SendEvent(&axis->motor.sm_mot, EVT_MOT_FORWARD);
			EventGroup_Clear(axis->motor.evt_mot, EVT_MOT_FORWARD);
			break;
		case EVT_MOT_REVERSE:    // 反转
			StateMachine_SendEvent(&axis->motor.sm_mot, EVT_MOT_REVERSE);
			EventGroup_Clear(axis->motor.evt_mot, EVT_MOT_REVERSE);
			break;
		case EVT_MOT_BRAKE:      // 制动
			StateMachine_SendEvent(&axis->motor.sm_mot, EVT_MOT_BRAKE);
			EventGroup_Clear(axis->motor.evt_mot, EVT_MOT_BRAKE);
			break;
		case EVT_MOT_STOP:      // 停止
			StateMachine_SendEvent(&axis->motor.sm_mot, EVT_MOT_STOP);
			EventGroup_Clear(axis->motor.evt_mot, EVT_MOT_STOP);
			break;
		case EVT_MOT_ERROR:      // 错误
			StateMachine_SendEvent(&axis->motor.sm_mot, EVT_MOT_ERROR);
			EventGroup_Clear(axis->motor.evt_mot, EVT_MOT_ERROR);
			break;
		case EVT_MOT_RESET:      // 复位
			StateMachine_SendEvent(&axis->motor.sm_mot, EVT_MOT_RESET);
			EventGroup_Clear(axis->motor.evt_mot, EVT_MOT_RESET);
			break;
		default:
			break;
    }
	EventGroup_ClearAll(axis->motor.evt_mot);
}

/* 电机驱动状态入口 */
static void mot_enter_idle(void)       {

}
static void mot_enter_run(void)    {

	uint8_t j = mySystem.target_id;

	if ( j < MAX_AXIS_NUM)
//    for (uint8_t j = 0; j < MOT_INDEX_MAX; j++)
	{
        Motor_t *mot = &mySystem.axis[j].motor;
        // 切换 电机方向
        switch(mot->sm_mot.cur_state) {
            case MOT_STATE_IDLE:
                mot->dir =  MOT_DIR_NONE;
                mot->enable = false;
                break;
            case MOT_STATE_FORWARD:
                mot->dir =  MOT_DIR_FORWARD;
                mot->enable = true;
                break;
            case MOT_STATE_REVERSE:
                mot->dir =  MOT_DIR_REVERSE;
                mot->enable = true;
                break;
            case MOT_STATE_STOP:      // 上下桥臂 均 关闭
                mot->dir =  MOT_DIR_NONE;
                mot->enable = false;
                break;
            case MOT_STATE_BRAKE:     // 上桥臂 均关闭；下桥臂 均开启
                mot->dir =  MOT_DIR_NONE;
                mot->enable = false;
                break;
            case MOT_STATE_ERROR:     // 上桥臂 均关闭；下桥臂 均开启
                mot->dir =  MOT_DIR_NONE;
                mot->enable = false;
                break;
            default:
                break;
        }
    }
}
// static void mot_enter_reverse(void)    {

// }
// static void mot_enter_brake(void)      {

// }
// static void mot_enter_stop(void)       {

// }
// static void mot_enter_error(void)      {

// }
