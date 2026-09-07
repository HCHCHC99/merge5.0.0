/**
 * @file    act_state.c
 * @brief   推杆状态机实现
 * @note    负责推杆运动逻辑，下发指令至电机
 */
#include "act_state.h"
#include "key_state.h"
#include "event_def.h"
#include "msg_pubsub.h"
#include "axis_typedef.h"
#include "hc32_ll_utility.h" //260609_RL_add:
#include "led.h"
#include "flash_mcu.h"
#include "pid_common.h"
#include "power_module.h"
#include "soft_timer.h"
#include "work_config.h"
#include "math.h"
#include "log_rtt.h"
#include "app_can.h"
#include "ctrl_current.h"

/* 全局系统对象 */
extern System_t mySystem;

///**
// * @brief 推杆状态枚举（完全私有，对外不可见）
// */
//typedef enum {
//    ACT_STATE_INIT = 0,                   // 初始化状态
//    ACT_STATE_IDLE ,                      // 空闲状态
//
//    ACT_STATE_COMBINE = 2,
//    ACT_STATE_COMBINE_TRY_1,
//    ACT_STATE_COMBINE_TRY_2,
//    ACT_STATE_COMBINE_TRY_3,
//    ACT_STATE_COMBINE_BACK = 6,
//    ACT_STATE_COMBINE_BACK_1,
//    ACT_STATE_COMBINE_BACK_2,
//    ACT_STATE_COMBINE_SUCCESS,
//    ACT_STATE_COMBINE_FAIL = 10,

//
//    ACT_STATE_SEPARATE =11,
//    ACT_STATE_SEPARATE_TRY_1,
//    ACT_STATE_SEPARATE_TRY_2,
//    ACT_STATE_SEPARATE_TRY_3,
//    ACT_STATE_SEPARATE_BACK = 15,
//    ACT_STATE_SEPARATE_BACK_1,
//    ACT_STATE_SEPARATE_BACK_2,
//    ACT_STATE_SEPARATE_SUCCESS,
//    ACT_STATE_SEPARATE_FAIL = 19,

//    ACT_STATE_DISABLE = 20,
//    ACT_STATE_HOLD,                     // 保持/制动
//    ACT_STATE_ERROR = 21,               // 错误
//
//    ACT_STATE_CALIBRATION = 22,             //260616_RL_add：标定
//    ACT_STATE_CALIBRATION_OUT,              //260616_RL_add：标定伸出
//    ACT_STATE_CALIBRATION_IN,               //260616_RL_add：标定伸出
//} ActuatorState_t;

/* 状态入口函数声明 */
static void act_enter_idle(void);
static void act_enter_combine(void);
static void act_enter_moving(void);
static void act_enter_separate(void);
static void act_enter_hold(void);
// static void act_enter_stop(void);
static void act_enter_calibration(void);   //260606_RL_add: 标定执行函数

static void act_exit(void);

//260713——add:
/* ===================== 软件定时器句柄（静态注册，4路单次定时器） ===================== */
static SoftTimer_Handle_t Act_htim_m1_enter;    // M1进入遇阻1.2s计时
static SoftTimer_Handle_t Act_htim_m2_enter;    // M2进入遇阻1.2s计时

static SoftTimer_Handle_t Act_htim_m1_exit;    // M1退出遇阻1.2s计时
static SoftTimer_Handle_t Act_htim_m2_exit;    // M2退出遇阻1.2s计时

static SoftTimer_Handle_t act_htim_m1_fault;    // M1高档位结合故障
static SoftTimer_Handle_t act_htim_m2_fault;    // M2高档位结合故障

/* ===================== 软定时器回调函数声明 ===================== */
static void Act_TimerCb_M1Enter(SoftTimer_Handle_t htimer);
static void Act_TimerCb_M1Exit(SoftTimer_Handle_t htimer);
static void Act_TimerCb_M2Enter(SoftTimer_Handle_t htimer);
static void Act_TimerCb_M2Exit(SoftTimer_Handle_t htimer);

static void Act_TimerCb_m1_fault(SoftTimer_Handle_t htimer);
static void Act_TimerCb_m2_fault(SoftTimer_Handle_t htimer);

#define  ACT_BACK_MS      (1200)    //推杆遇阻回退时间

/**
 * @brief 推杆状态跳转表（全枚举定义）
 */
static const StateJumpTable_t act_jump[] = {

    {ACT_STATE_INIT,             EVT_ACT_WORK_ENABLE,      ACT_STATE_IDLE},

    {ACT_STATE_IDLE,             EVT_ACT_WORK_DISABLE,     ACT_STATE_DISABLE},
    {ACT_STATE_IDLE,             EVT_ACT_COMBINE,          ACT_STATE_COMBINE},
    {ACT_STATE_IDLE,             EVT_ACT_SEPARATE,         ACT_STATE_SEPARATE},
    {ACT_STATE_IDLE,             EVT_ACT_HOLD,             ACT_STATE_HOLD},
    {ACT_STATE_IDLE,             EVT_ACT_ERROR,            ACT_STATE_ERROR},

    {ACT_STATE_COMBINE,          EVT_ACT_SEPARATE,         ACT_STATE_SEPARATE},
    {ACT_STATE_COMBINE,          EVT_ACT_COMBINE_SUCCESS,  ACT_STATE_COMBINE_SUCCESS},
    {ACT_STATE_COMBINE,          EVT_ACT_COMBINE_BACK,     ACT_STATE_COMBINE_BACK},
    {ACT_STATE_COMBINE,          EVT_ACT_HOLD,             ACT_STATE_HOLD},
    {ACT_STATE_COMBINE,          EVT_ACT_ERROR,            ACT_STATE_ERROR},
    {ACT_STATE_COMBINE,          EVT_ACT_WORK_DISABLE,     ACT_STATE_DISABLE},
    {ACT_STATE_COMBINE, 		EVT_ACT_RESET,            ACT_STATE_IDLE},

    {ACT_STATE_COMBINE_BACK,     EVT_ACT_SEPARATE,         ACT_STATE_SEPARATE},
    {ACT_STATE_COMBINE_BACK,     EVT_ACT_COMBINE,          ACT_STATE_COMBINE_TRY_1},
    {ACT_STATE_COMBINE_BACK,     EVT_ACT_HOLD,             ACT_STATE_HOLD},
    {ACT_STATE_COMBINE_BACK,     EVT_ACT_ERROR,            ACT_STATE_ERROR},
    {ACT_STATE_COMBINE_BACK,     EVT_ACT_WORK_DISABLE,     ACT_STATE_DISABLE},
    {ACT_STATE_COMBINE_BACK, 		EVT_ACT_RESET,            ACT_STATE_IDLE},

    {ACT_STATE_COMBINE_TRY_1,    EVT_ACT_SEPARATE,         ACT_STATE_SEPARATE},
    {ACT_STATE_COMBINE_TRY_1,    EVT_ACT_COMBINE_BACK,     ACT_STATE_COMBINE_BACK_1},
    {ACT_STATE_COMBINE_TRY_1,    EVT_ACT_HOLD,             ACT_STATE_HOLD},
    {ACT_STATE_COMBINE_TRY_1,    EVT_ACT_ERROR,            ACT_STATE_ERROR},
    {ACT_STATE_COMBINE_TRY_1,    EVT_ACT_WORK_DISABLE,     ACT_STATE_DISABLE},
    {ACT_STATE_COMBINE_TRY_1,    EVT_ACT_COMBINE_SUCCESS,  ACT_STATE_COMBINE_SUCCESS},      //260611_RL_add:尝试时可以成功
    {ACT_STATE_COMBINE_TRY_1, 		EVT_ACT_RESET,            ACT_STATE_IDLE},

    {ACT_STATE_COMBINE_BACK_1,   EVT_ACT_SEPARATE,         ACT_STATE_SEPARATE},
    {ACT_STATE_COMBINE_BACK_1,   EVT_ACT_COMBINE,          ACT_STATE_COMBINE_TRY_2},
    {ACT_STATE_COMBINE_BACK_1,   EVT_ACT_HOLD,             ACT_STATE_HOLD},
    {ACT_STATE_COMBINE_BACK_1,   EVT_ACT_ERROR,            ACT_STATE_ERROR},
    {ACT_STATE_COMBINE_BACK_1,   EVT_ACT_WORK_DISABLE,     ACT_STATE_DISABLE},
    {ACT_STATE_COMBINE_BACK_1, 		EVT_ACT_RESET,            ACT_STATE_IDLE},

    {ACT_STATE_COMBINE_TRY_2,    EVT_ACT_SEPARATE,         ACT_STATE_SEPARATE},
    {ACT_STATE_COMBINE_TRY_2,    EVT_ACT_COMBINE_BACK,     ACT_STATE_COMBINE_BACK_2},
    {ACT_STATE_COMBINE_TRY_2,    EVT_ACT_HOLD,             ACT_STATE_HOLD},
    {ACT_STATE_COMBINE_TRY_2,    EVT_ACT_ERROR,            ACT_STATE_ERROR},
    {ACT_STATE_COMBINE_TRY_2,    EVT_ACT_WORK_DISABLE,     ACT_STATE_DISABLE},
    {ACT_STATE_COMBINE_TRY_2,    EVT_ACT_COMBINE_SUCCESS,  ACT_STATE_COMBINE_SUCCESS},      //260611_RL_add:尝试时可以成功
    {ACT_STATE_COMBINE_TRY_2, 		EVT_ACT_RESET,            ACT_STATE_IDLE},

    {ACT_STATE_COMBINE_BACK_2,   EVT_ACT_SEPARATE,         ACT_STATE_SEPARATE},
    {ACT_STATE_COMBINE_BACK_2,   EVT_ACT_COMBINE,          ACT_STATE_COMBINE_FAIL}, //260708——add：回退1.2s后就要停止，先这样写事件和跳转
    {ACT_STATE_COMBINE_BACK_2,   EVT_ACT_HOLD,             ACT_STATE_HOLD},
    {ACT_STATE_COMBINE_BACK_2,   EVT_ACT_ERROR,            ACT_STATE_ERROR},
    {ACT_STATE_COMBINE_BACK_2,   EVT_ACT_WORK_DISABLE,     ACT_STATE_DISABLE},
    {ACT_STATE_COMBINE_BACK_2, 		EVT_ACT_RESET,            ACT_STATE_IDLE},

    {ACT_STATE_SEPARATE,         EVT_ACT_COMBINE,          ACT_STATE_COMBINE},
    {ACT_STATE_SEPARATE,         EVT_ACT_SEPARATE_SUCCESS, ACT_STATE_SEPARATE_SUCCESS},
    {ACT_STATE_SEPARATE,         EVT_ACT_SEPARATE_BACK,    ACT_STATE_SEPARATE_BACK},
    {ACT_STATE_SEPARATE,         EVT_ACT_HOLD,             ACT_STATE_HOLD},
    {ACT_STATE_SEPARATE,         EVT_ACT_ERROR,            ACT_STATE_ERROR},
    {ACT_STATE_SEPARATE,         EVT_ACT_WORK_DISABLE,     ACT_STATE_DISABLE},
    {ACT_STATE_SEPARATE, 		EVT_ACT_RESET,            ACT_STATE_IDLE},

    {ACT_STATE_SEPARATE_BACK,    EVT_ACT_COMBINE,          ACT_STATE_COMBINE},
    {ACT_STATE_SEPARATE_BACK,    EVT_ACT_SEPARATE,         ACT_STATE_SEPARATE_TRY_1},
    {ACT_STATE_SEPARATE_BACK,    EVT_ACT_HOLD,             ACT_STATE_HOLD},
    {ACT_STATE_SEPARATE_BACK,    EVT_ACT_ERROR,            ACT_STATE_ERROR},
    {ACT_STATE_SEPARATE_BACK,    EVT_ACT_WORK_DISABLE,     ACT_STATE_DISABLE},
    {ACT_STATE_SEPARATE_BACK, 		EVT_ACT_RESET,            ACT_STATE_IDLE},

    {ACT_STATE_SEPARATE_TRY_1,   EVT_ACT_COMBINE,          ACT_STATE_COMBINE},
    {ACT_STATE_SEPARATE_TRY_1,   EVT_ACT_SEPARATE_BACK,    ACT_STATE_SEPARATE_BACK_1},
    {ACT_STATE_SEPARATE_TRY_1,   EVT_ACT_HOLD,             ACT_STATE_HOLD},
    {ACT_STATE_SEPARATE_TRY_1,   EVT_ACT_ERROR,            ACT_STATE_ERROR},
    {ACT_STATE_SEPARATE_TRY_1,   EVT_ACT_WORK_DISABLE,     ACT_STATE_DISABLE},
    {ACT_STATE_SEPARATE_TRY_1,   EVT_ACT_SEPARATE_SUCCESS, ACT_STATE_SEPARATE_SUCCESS},      //260611_RL_add:尝试时可以成功
    {ACT_STATE_SEPARATE_TRY_1, 		EVT_ACT_RESET,            ACT_STATE_IDLE},

    {ACT_STATE_SEPARATE_BACK_1,  EVT_ACT_COMBINE,          ACT_STATE_COMBINE},
    {ACT_STATE_SEPARATE_BACK_1,  EVT_ACT_SEPARATE,         ACT_STATE_SEPARATE_TRY_2},
    {ACT_STATE_SEPARATE_BACK_1,  EVT_ACT_HOLD,             ACT_STATE_HOLD},
    {ACT_STATE_SEPARATE_BACK_1,  EVT_ACT_ERROR,            ACT_STATE_ERROR},
    {ACT_STATE_SEPARATE_BACK_1,  EVT_ACT_WORK_DISABLE,     ACT_STATE_DISABLE},
    {ACT_STATE_SEPARATE_BACK_1, 		EVT_ACT_RESET,            ACT_STATE_IDLE},

    {ACT_STATE_SEPARATE_TRY_2,   EVT_ACT_COMBINE,          ACT_STATE_COMBINE},
    {ACT_STATE_SEPARATE_TRY_2,   EVT_ACT_SEPARATE_BACK,    ACT_STATE_SEPARATE_BACK_2},
    {ACT_STATE_SEPARATE_TRY_2,   EVT_ACT_HOLD,             ACT_STATE_HOLD},
    {ACT_STATE_SEPARATE_TRY_2,   EVT_ACT_ERROR,            ACT_STATE_ERROR},
    {ACT_STATE_SEPARATE_TRY_2,   EVT_ACT_WORK_DISABLE,     ACT_STATE_DISABLE},
    {ACT_STATE_SEPARATE_TRY_2,   EVT_ACT_SEPARATE_SUCCESS, ACT_STATE_SEPARATE_SUCCESS},      //260611_RL_add:尝试时可以成功
    {ACT_STATE_SEPARATE_TRY_2, 		EVT_ACT_RESET,            ACT_STATE_IDLE},

    {ACT_STATE_SEPARATE_BACK_2,  EVT_ACT_COMBINE,          ACT_STATE_COMBINE},
    {ACT_STATE_SEPARATE_BACK_2,  EVT_ACT_SEPARATE,         ACT_STATE_SEPARATE_FAIL},   //260708——add：回退1.2s后就要停止，先这样写事件和跳转
    {ACT_STATE_SEPARATE_BACK_2,  EVT_ACT_HOLD,             ACT_STATE_HOLD},
    {ACT_STATE_SEPARATE_BACK_2,  EVT_ACT_ERROR,            ACT_STATE_ERROR},
    {ACT_STATE_SEPARATE_BACK_2,  EVT_ACT_WORK_DISABLE,     ACT_STATE_DISABLE},
    {ACT_STATE_SEPARATE_BACK_2, 		EVT_ACT_RESET,            ACT_STATE_IDLE},

    {ACT_STATE_HOLD,             EVT_ACT_WORK_ENABLE,      ACT_STATE_IDLE},
    {ACT_STATE_ERROR,            EVT_ACT_RESET,            ACT_STATE_INIT},
    {ACT_STATE_DISABLE,          EVT_ACT_WORK_ENABLE,      ACT_STATE_IDLE},

    //260611_RL_add: 测试方便，增加结合成功/失败,发送复位事件，跳到空闲状态。 用完根据实际情况修改、删除！
    {ACT_STATE_SEPARATE_SUCCESS, EVT_ACT_RESET,            ACT_STATE_IDLE},
    {ACT_STATE_COMBINE_SUCCESS,  EVT_ACT_RESET,            ACT_STATE_IDLE},
    {ACT_STATE_COMBINE_FAIL,     EVT_ACT_RESET,            ACT_STATE_IDLE},
    {ACT_STATE_SEPARATE_FAIL,    EVT_ACT_RESET,            ACT_STATE_IDLE},

	{ACT_STATE_IDLE,             EVT_ACT_OVERSPEED_ENABLE, ACT_STATE_OVERSPEED},                                   // IDLE 状态 + 超速事件触发 -> 超速状态
	{ACT_STATE_COMBINE,          EVT_ACT_OVERSPEED_ENABLE, ACT_STATE_OVERSPEED_ALARM},                //  ACT_STATE_COMBINE 状态 + 超速事件触发 -> 超速报警状态
	{ACT_STATE_COMBINE_BACK,     EVT_ACT_OVERSPEED_ENABLE, ACT_STATE_OVERSPEED_ALARM},           //  ACT_STATE_COMBINE_BACK 状态 + 超速事件触发 -> 超速报警状态
	{ACT_STATE_COMBINE_TRY_1,    EVT_ACT_OVERSPEED_ENABLE, ACT_STATE_OVERSPEED_ALARM},           //  ACT_STATE_COMBINE_TRY_1 状态 + 超速事件触发 -> 超速报警状态
	{ACT_STATE_COMBINE_BACK_1,   EVT_ACT_OVERSPEED_ENABLE, ACT_STATE_OVERSPEED_ALARM},           //  ACT_STATE_COMBINE_BACK_1 状态 + 超速事件触发 -> 超速报警状态
	{ACT_STATE_COMBINE_TRY_2,    EVT_ACT_OVERSPEED_ENABLE, ACT_STATE_OVERSPEED_ALARM},           //  ACT_STATE_COMBINE_TRY_2 状态 + 超速事件触发 -> 超速报警状态
	{ACT_STATE_COMBINE_BACK_2,   EVT_ACT_OVERSPEED_ENABLE, ACT_STATE_OVERSPEED_ALARM},           //  ACT_STATE_COMBINE_BACK_2 状态 + 超速事件触发 -> 超速报警状态
	{ACT_STATE_SEPARATE,         EVT_ACT_OVERSPEED_ENABLE, ACT_STATE_OVERSPEED_ALARM},                  //  ACT_STATE_SEPARATE 状态 + 超速事件触发 -> 超速报警状态
	{ACT_STATE_SEPARATE_BACK,    EVT_ACT_OVERSPEED_ENABLE, ACT_STATE_OVERSPEED_ALARM},            //  ACT_STATE_SEPARATE_BACK 状态 + 超速事件触发 -> 超速报警状态
	{ACT_STATE_SEPARATE_TRY_1,   EVT_ACT_OVERSPEED_ENABLE, ACT_STATE_OVERSPEED_ALARM},            //  ACT_STATE_SEPARATE_TRY_1 状态 + 超速事件触发 -> 超速报警状态
	{ACT_STATE_SEPARATE_BACK_1,  EVT_ACT_OVERSPEED_ENABLE, ACT_STATE_OVERSPEED_ALARM},            //  ACT_STATE_SEPARATE_BACK_1 状态 + 超速事件触发 -> 超速报警状态
	{ACT_STATE_SEPARATE_TRY_2,   EVT_ACT_OVERSPEED_ENABLE, ACT_STATE_OVERSPEED_ALARM},             //  ACT_STATE_SEPARATE_TRY_2 状态 + 超速事件触发 -> 超速报警状态
	{ACT_STATE_SEPARATE_BACK_2,  EVT_ACT_OVERSPEED_ENABLE, ACT_STATE_OVERSPEED_ALARM},           //  ACT_STATE_SEPARATE_BACK_2 状态 + 超速事件触发 -> 超速报警状态

	{ACT_STATE_OVERSPEED,          EVT_ACT_COMBINE,     ACT_STATE_OVERSPEED_ALARM },              // 超速状态 + 结合事件  -> 超速报警状态
	{ACT_STATE_OVERSPEED,          EVT_ACT_SEPARATE,    ACT_STATE_OVERSPEED_ALARM },               // 超速状态 + 分离事件  -> 超速报警状态
	{ACT_STATE_OVERSPEED,          EVT_ACT_OVERSPEED_DISABLE,    ACT_STATE_IDLE },             // 超速状态 + 超速失能 事件  -> 超速报警状态
    {ACT_STATE_OVERSPEED, 		EVT_ACT_RESET,            ACT_STATE_IDLE},
    {ACT_STATE_OVERSPEED, 		EVT_ACT_HGEAR_COMB_ED,            ACT_STATE_H_GEAR_COMB_ED},//260828

	{ACT_STATE_OVERSPEED_ALARM,    EVT_ACT_OVERSPEED_ALARM_DISABLE,     ACT_STATE_OVERSPEED },    // 超速报警 状态 + 报警失能 事件  -> 超速状态
    {ACT_STATE_OVERSPEED_ALARM, 		EVT_ACT_RESET,            ACT_STATE_IDLE},

     //高档位和高档位报警
	{ACT_STATE_IDLE,      			EVT_ACT_HGEAR_NO_COMB,      ACT_STATE_H_GEAR_NO_COMB },
	{ACT_STATE_IDLE,     			EVT_ACT_HGEAR_COMB_ED,      ACT_STATE_H_GEAR_COMB_ED },

	{ACT_STATE_H_GEAR_COMB_ED,     	EVT_ACT_HGEAR_CANNEL,		ACT_STATE_IDLE },
	{ACT_STATE_H_GEAR_COMB_ED,      EVT_ACT_SEPARATE,			ACT_STATE_SEPARATE },
    {ACT_STATE_H_GEAR_COMB_ED, 		EVT_ACT_RESET,            ACT_STATE_IDLE},

	{ACT_STATE_H_GEAR_NO_COMB,      EVT_ACT_HGEAR_CANNEL,      	ACT_STATE_IDLE },
	{ACT_STATE_H_GEAR_NO_COMB,      EVT_ACT_COMBINE,      		ACT_STATE_H_GEAR_COMB_5S },
	{ACT_STATE_H_GEAR_NO_COMB,      EVT_ACT_SEPARATE,      		ACT_STATE_SEPARATE },
    {ACT_STATE_H_GEAR_NO_COMB, 		EVT_ACT_RESET,            ACT_STATE_IDLE},

	{ACT_STATE_COMBINE,       		EVT_ACT_HGEAR_NO_COMB,      ACT_STATE_H_GEAR_COMB_5S },
	{ACT_STATE_COMBINE_TRY_1,		EVT_ACT_HGEAR_NO_COMB,      ACT_STATE_H_GEAR_COMB_5S },
	{ACT_STATE_COMBINE_TRY_2,		EVT_ACT_HGEAR_NO_COMB,      ACT_STATE_H_GEAR_COMB_5S },
	{ACT_STATE_COMBINE_BACK,       	EVT_ACT_HGEAR_NO_COMB,      ACT_STATE_H_GEAR_COMB_5S },
	{ACT_STATE_COMBINE_BACK_1,		EVT_ACT_HGEAR_NO_COMB,      ACT_STATE_H_GEAR_COMB_5S },
	{ACT_STATE_COMBINE_BACK_2,		EVT_ACT_HGEAR_NO_COMB,      ACT_STATE_H_GEAR_COMB_5S },

	{ACT_STATE_H_GEAR_COMB_5S,		EVT_ACT_HGEAR_COMB_EXIT,      ACT_STATE_H_GEAR_NO_COMB },
    {ACT_STATE_H_GEAR_COMB_5S, 		EVT_ACT_RESET,            ACT_STATE_IDLE},
};

/*
测试方便查看事件，测完删除
--------------------- 推杆事件组 事件定义---------------------
#define EVT_ACT_WORK_ENABLE            (1U << 0)    // 推杆 工作使能 1
#define EVT_ACT_WORK_DISABLE           (1U << 1)    // 推杆 工作禁止 2

#define EVT_ACT_COMBINE                (1U << 2)    // 推杆 结合    4
#define EVT_ACT_COMBINE_TRY_1          (1U << 3)    // 推杆 结合 尝试1 8
#define EVT_ACT_COMBINE_TRY_2          (1U << 4)    // 推杆 结合 尝试2 16
#define EVT_ACT_COMBINE_TRY_3          (1U << 5)    // 推杆 结合 尝试3 32
#define EVT_ACT_COMBINE_BACK           (1U << 6)    // 推杆 结合 回退  64
#define EVT_ACT_COMBINE_SUCCESS        (1U << 7)    // 推杆 结合 成功  128
#define EVT_ACT_COMBINE_FAIL           (1U << 8)    // 推杆 结合 失败  256

#define EVT_ACT_SEPARATE               (1U << 9)    // 推杆 分离    512
#define EVT_ACT_SEPARATE_TRY_1         (1U << 10)    // 推杆 分离 尝试1 1024
#define EVT_ACT_SEPARATE_TRY_2         (1U << 11)    // 推杆 分离 尝试2 2048
#define EVT_ACT_SEPARATE_TRY_3         (1U << 12)    // 推杆 分离 尝试3 4096
#define EVT_ACT_SEPARATE_BACK          (1U << 13)    // 推杆 分离 回退  8192
#define EVT_ACT_SEPARATE_SUCCESS       (1U << 14)    // 推杆 分离 成功  16384
#define EVT_ACT_SEPARATE_FAIL          (1U << 15)    // 推杆 分离 失败 32768

#define EVT_ACT_HOLD                   (1U << 16)     // 推杆 保持 65536
#define EVT_ACT_ERROR                  (1U << 17)     // 推杆 错误 131072
#define EVT_ACT_RESET                  (1U << 18)     // 推杆 复位 262144

#define EVT_ACT_HOLD                   (1U << 16)     // 推杆 保持 65536
#define EVT_ACT_ERROR                  (1U << 17)     // 推杆 错误 131072
#define EVT_ACT_RESET                  (1U << 18)     // 推杆 复位 262144

#define EVT_ACT_CALIBRATION_START      (1U << 19)     // 推杆 开始标定 524288
#define EVT_ACT_CALIBRATION_OUT        (1U << 20)     // 推杆 标定伸出 1048576
#define EVT_ACT_CALIBRATION_IN         (1U << 21)     // 推杆 停止缩回 2097152
#define EVT_ACT_CALIBRATION_STOP       (1U << 22)     // 推杆 停止标定 4194304

**/

/**
 * @brief 推杆状态入口函数表
 */
static const StateFuncTable_t act_func[] = {
   {ACT_STATE_INIT,                act_enter_idle,        0},
   {ACT_STATE_IDLE,                act_enter_idle,        0},

   {ACT_STATE_COMBINE,             act_enter_moving,      0},
   {ACT_STATE_COMBINE_TRY_1,       act_enter_moving,      0},
   {ACT_STATE_COMBINE_TRY_2,       act_enter_moving,      0},
//    {ACT_STATE_COMBINE_TRY_3,       act_enter_moving,      0},
   {ACT_STATE_COMBINE_BACK,        act_enter_moving,      0},
   {ACT_STATE_COMBINE_BACK_1,      act_enter_moving,      0},
   {ACT_STATE_COMBINE_BACK_2,      act_enter_moving,      0},
   {ACT_STATE_COMBINE_SUCCESS,     act_enter_idle,        0},
   {ACT_STATE_COMBINE_FAIL,        act_enter_idle,        0},

   {ACT_STATE_SEPARATE,            act_enter_moving,      0},
   {ACT_STATE_SEPARATE_TRY_1,      act_enter_moving,      0},
   {ACT_STATE_SEPARATE_TRY_2,      act_enter_moving,      0},
//    {ACT_STATE_SEPARATE_TRY_3,      act_enter_moving,      0},
   {ACT_STATE_SEPARATE_BACK,       act_enter_moving,      0},
   {ACT_STATE_SEPARATE_BACK_1,     act_enter_moving,      0},
   {ACT_STATE_SEPARATE_BACK_2,     act_enter_moving,      0},
   {ACT_STATE_SEPARATE_SUCCESS,    act_enter_idle,        0},
   {ACT_STATE_SEPARATE_FAIL,       act_enter_idle,        0},

   {ACT_STATE_DISABLE,             act_enter_hold,        0},
   {ACT_STATE_HOLD,                act_enter_hold,        0},
   {ACT_STATE_ERROR,               act_enter_hold,        0},

    //260709-add:增加超速、高档位状态的入口函数——需要确认这种报警状态 推杆的执行动作
    {ACT_STATE_H_GEAR_NO_COMB,     act_enter_hold,        act_exit},
    {ACT_STATE_H_GEAR_COMB_5S,    act_enter_hold,        act_exit},
    {ACT_STATE_H_GEAR_COMB_ED,     act_enter_hold,        act_exit},
    {ACT_STATE_OVERSPEED,   	 act_enter_hold,        act_exit},
    {ACT_STATE_OVERSPEED_ALARM,     act_enter_hold,        act_exit},

};

void act_led_ctrl(Axis_t *axis)
{

	// 推杆正常模式下，根据推杆的位置，控制led显示不同颜色
    if(axis->mode == ACT_MODE_NORMAL  || axis->mode == ACT_MODE_IDLE  ){

		switch(axis->sm_act.cur_state)
		{
			case ACT_STATE_IDLE:
			case ACT_STATE_COMBINE_SUCCESS:
			case ACT_STATE_SEPARATE_SUCCESS:
			case ACT_STATE_OVERSPEED:
			case ACT_STATE_H_GEAR_NO_COMB:
				if (ACT_STATE_COMBINE_FAIL == axis->sm_act.previous_state ||
					ACT_STATE_SEPARATE_FAIL == axis->sm_act.previous_state)
				{
					led_set_event(axis->id, (EVT_LEDx_RED_ON | EVT_LEDx_BLINK_LOOP | EVT_LEDx_FREQ_3xHZ_50 | EVT_LEDx_PRIORITY_4));
				}else
				{
					// 如果推杆未标定 且 acc 使能
					if(axis->acc_enable){
							// 控制指示灯
								if(axis->position == ACT_POS_COMBINE){
										led_set_event(axis->id, (EVT_LEDx_GREEN_ON | EVT_LEDx_PRIORITY_5));
								}else if(axis->position == ACT_POS_SEPARATE){
										led_set_event(axis->id, (EVT_LEDx_YELLOW_ON | EVT_LEDx_PRIORITY_5));
								}else{
									if (POS_ERR == axis->pos_err)
									{
										led_set_event(axis->id, (EVT_LEDx_RED_YELLOW_ON | EVT_LEDx_PRIORITY_5));
									}
									else
									{
										led_set_event(axis->id,(EVT_LEDx_RED_ON | EVT_LEDx_FREQ_3xHZ_50 | EVT_LEDx_BLINK_LOOP | EVT_LEDx_PRIORITY_5));
									}
							}

					}else if(!axis->acc_enable){
									led_set_event(axis->id, EVT_LEDx_DISABLE);
					}
				}
			break;

			case ACT_STATE_OVERSPEED_ALARM:
			case ACT_STATE_H_GEAR_COMB_5S:
			case ACT_STATE_H_GEAR_COMB_ED:
				led_set_event(axis->id,(EVT_LEDx_RED_ON | EVT_LEDx_FREQ_3xHZ_50 | EVT_LEDx_BLINK_LOOP | EVT_LEDx_PRIORITY_4));
			break;

			case ACT_STATE_COMBINE_BACK_2:
			case ACT_STATE_SEPARATE_BACK_2:
				led_set_event(axis->id,(EVT_LEDx_RED_ON | EVT_LEDx_FREQ_3xHZ_50 | EVT_LEDx_BLINK_LOOP | EVT_LEDx_PRIORITY_4));
			break;

			case ACT_STATE_COMBINE:
            case ACT_STATE_COMBINE_TRY_1:
            case ACT_STATE_COMBINE_TRY_2:
            case ACT_STATE_SEPARATE_BACK:
            case ACT_STATE_SEPARATE_BACK_1:
            case ACT_STATE_SEPARATE:
            case ACT_STATE_SEPARATE_TRY_1:
            case ACT_STATE_SEPARATE_TRY_2:
            case ACT_STATE_COMBINE_BACK:
            case ACT_STATE_COMBINE_BACK_1:
			//绿灯闪
				led_set_event(axis->id, (EVT_LEDx_FREQ_2HZ_50 | EVT_LEDx_BLINK_LOOP | EVT_LEDx_GREEN_ON | EVT_LEDx_PRIORITY_5));
             break;
			case ACT_STATE_DISABLE:
				led_set_event(axis->id, EVT_LEDx_DISABLE);

            default:
             break;

		}

		if (ACT_STATE_OVERSPEED_ALARM == axis->sm_act.cur_state ||
			ACT_STATE_H_GEAR_COMB_5S == axis->sm_act.cur_state ||
			ACT_STATE_H_GEAR_COMB_ED == axis->sm_act.cur_state )
		{
			led_set_event(axis->id,(EVT_LEDx_RED_ON | EVT_LEDx_FREQ_3xHZ_50 | EVT_LEDx_BLINK_LOOP | EVT_LEDx_PRIORITY_4));
		}

		if (CALIB_STATE_INTO_MANU_ING == axis->sm_calib.cur_state)
		{
			led_set_event(axis->id,(EVT_LEDx_RED_YELLOW_ON | EVT_LEDx_FREQ_10HZ_50 | EVT_LEDx_BLINK_LOOP | EVT_LEDx_PRIORITY_1));
		}
	}
}

void gear_check(Axis_t *axis_t)
{
//	System_t *mySystem_t = container_of_arr_any(axis_t, System_t, axis);
//
//	if (mySystem_t->key_over_gear_enable || mySystem_t->can_over_gear_enable){
//		EventGroup_Send(axis_t->evt_act, EVT_ACT_HGEAR_NO_COMB);
//	}else{
//		EventGroup_Send(axis_t->evt_act, EVT_ACT_HGEAR_CANNEL);
//	}
	if (mySystem.key_over_gear_enable || mySystem.can_over_gear_enable){
		EventGroup_Send(axis_t->evt_act, EVT_ACT_HGEAR_NO_COMB);
	}else{
		EventGroup_Send(axis_t->evt_act, EVT_ACT_HGEAR_CANNEL);
	}

}

void speed_check(Axis_t *axis)
{
	if (mySystem.over_spd_enable){
		EventGroup_Send(axis->evt_act, EVT_ACT_OVERSPEED_ENABLE);
	}else{
		EventGroup_Send(axis->evt_act, EVT_ACT_OVERSPEED_DISABLE);
	}

}

/**
 * @brief 推杆状态机 初始化
 * @note
 */
void Act_State_Init(StateMachine_t *sm)
{
    StateMachine_t *sm_temp = sm;
    sm_temp->jump_table = act_jump;
    sm_temp->jump_table_size = sizeof(act_jump)/sizeof(StateJumpTable_t);
    sm_temp->func_table = act_func;
    sm_temp->func_table_size = sizeof(act_func)/sizeof(StateFuncTable_t);
    sm_temp->init_state = ACT_STATE_IDLE;

    StateMachine_Init(sm_temp);

    //260713_add:增加软件定时器
        /* 注册4个单次软件定时器，绑定对应回调 */
    Act_htim_m1_enter = SoftTimer_Register();
    Act_htim_m1_exit  = SoftTimer_Register();
    Act_htim_m2_enter = SoftTimer_Register();
    Act_htim_m2_exit  = SoftTimer_Register();

    act_htim_m1_fault = SoftTimer_Register();
    act_htim_m2_fault  = SoftTimer_Register();

    if(Act_htim_m1_enter != NULL) SoftTimer_SetCallback(Act_htim_m1_enter, Act_TimerCb_M1Enter);
    if(Act_htim_m1_exit  != NULL) SoftTimer_SetCallback(Act_htim_m1_exit,  Act_TimerCb_M1Exit);
    if(Act_htim_m2_enter != NULL) SoftTimer_SetCallback(Act_htim_m2_enter, Act_TimerCb_M2Enter);
    if(Act_htim_m2_exit  != NULL) SoftTimer_SetCallback(Act_htim_m2_exit,  Act_TimerCb_M2Exit);

    if(act_htim_m1_fault != NULL) SoftTimer_SetCallback(act_htim_m1_fault, Act_TimerCb_m1_fault);
    if(act_htim_m2_fault  != NULL) SoftTimer_SetCallback(act_htim_m2_fault,  Act_TimerCb_m2_fault);
}
/**
 * @brief 推杆状态机运行
 * @param  axis: 轴对象
 * @note   处理事件，下发电动机指令
 */
void Act_State_Task(Axis_t *axis)
{
	if (ACT_DIR_NONE == mySystem.axis[axis->id].dir)
	{
		led_set_event(axis->id, EVT_LEDx_OFF);
		return;
	}

	act_led_ctrl(axis);

    //260709_add:增加判断系统状态:标定状态（返回到IDLE）；正常状态执行task
    //未标定模式、传感器异常时，推杆不能动（不能进入normal状态）
    //传感器异常：控制LED橘色常亮；未标定：红绿交替闪烁

    if( ACT_MODE_CALIB_MODE == axis->mode || CALIB_STATE_INTO_MANU_ING == axis->sm_calib.cur_state)      //推杆正处于标定模式 --|准备进入手动标定模式
    {
        return ;
    }
    else
    {
		if(POS_NO == axis->pos_err)    //传感器异常要补
        {
            //橘灯常亮——亮几个灯？——亮对应的灯
            led_set_event(axis->id, (EVT_LEDx_RED_YELLOW_ON | EVT_LEDx_PRIORITY_2));
			axis->pos_err_last = axis->pos_err;

            //返回
            return;
        }
        else   //传感器正常
        {
			if (axis->pos_err != axis->pos_err_last)//传感状态更新
			{
				led_set_event(axis->id, EVT_LEDx_OFF);
				axis->pos_err_last = axis->pos_err;
			}

			if(false == axis->is_calib)    //未标定
			{
				//红绿交替闪烁
				led_set_event(axis->id, (EVT_LEDx_RED_GREEN_ON|EVT_LEDx_BLINK_ALT | EVT_LEDx_PRIORITY_3));
				axis->is_calib_last = axis->is_calib;
				//返回
				return;
			}
			else  //已经标定
			{
				mySystem.axis[axis->id].mode = ACT_MODE_NORMAL;

				gear_check(axis);
				speed_check(axis);

				if (axis->is_calib != axis->is_calib_last)//传感状态更新
				{
					led_set_event(axis->id, EVT_LEDx_OFF);
					axis->is_calib_last = axis->is_calib;
				}

				if (EventGroup_Get(axis->evt_act) & EVT_ACT_HGEAR_NO_COMB)//判断位置
				{
					if ((ACT_STATE_IDLE == mySystem.axis[axis->id].sm_act.cur_state ||
						ACT_STATE_OVERSPEED == mySystem.axis[axis->id].sm_act.cur_state)
						&&
						ACT_POS_COMBINE == mySystem.axis[axis->id].position)
					{
						EventGroup_Clear(axis->evt_act, EVT_ACT_HGEAR_NO_COMB);
						EventGroup_Send(axis->evt_act, EVT_ACT_HGEAR_COMB_ED);
					}
				}

				EventBits_t bits = EventGroup_Get(axis->evt_act);

				if (bits & EVT_ACT_WORK_DISABLE)
				{
					StateMachine_SendEvent(&axis->sm_act, EVT_ACT_WORK_DISABLE);
					EventGroup_Clear(axis->evt_act, EVT_ACT_WORK_DISABLE);
					axis->enable = 0;
				}

				if (bits & EVT_ACT_WORK_ENABLE)
				{
					StateMachine_SendEvent(&axis->sm_act, EVT_ACT_WORK_ENABLE);
					EventGroup_Clear(axis->evt_act, EVT_ACT_WORK_ENABLE);
					axis->enable = 1;
				}

				if (bits & EVT_ACT_ERROR)
				{
					StateMachine_SendEvent(&axis->sm_act, EVT_ACT_ERROR);
					EventGroup_Clear(axis->evt_act, EVT_ACT_ERROR);
				}

				if (bits & EVT_ACT_HOLD)
				{
					StateMachine_SendEvent(&axis->sm_act, EVT_ACT_HOLD);
					EventGroup_Clear(axis->evt_act, EVT_ACT_HOLD);
				}

				if (bits & EVT_ACT_RESET)
				{
					StateMachine_SendEvent(&axis->sm_act, EVT_ACT_RESET);
					EventGroup_Clear(axis->evt_act, EVT_ACT_RESET);
				}

				if (bits & EVT_ACT_OVERSPEED_ENABLE)
				{
					StateMachine_SendEvent(&axis->sm_act, EVT_ACT_OVERSPEED_ENABLE);
					EventGroup_Clear(axis->evt_act, EVT_ACT_OVERSPEED_ENABLE);
				}

				if (bits & EVT_ACT_OVERSPEED_DISABLE)
				{
					StateMachine_SendEvent(&axis->sm_act, EVT_ACT_OVERSPEED_DISABLE);
					EventGroup_Clear(axis->evt_act, EVT_ACT_OVERSPEED_DISABLE);
				}

				if (bits & EVT_ACT_OVERSPEED_ALARM_DISABLE)
				{
					StateMachine_SendEvent(&axis->sm_act, EVT_ACT_OVERSPEED_ALARM_DISABLE);
					EventGroup_Clear(axis->evt_act, EVT_ACT_OVERSPEED_ALARM_DISABLE);
				}

				if (bits & EVT_ACT_HGEAR_COMB_ED)
				{
					StateMachine_SendEvent(&axis->sm_act, EVT_ACT_HGEAR_COMB_ED);
					EventGroup_Clear(axis->evt_act, EVT_ACT_HGEAR_COMB_ED);
				}

				if (bits & EVT_ACT_HGEAR_NO_COMB)
				{
					StateMachine_SendEvent(&axis->sm_act, EVT_ACT_HGEAR_NO_COMB);
					EventGroup_Clear(axis->evt_act, EVT_ACT_HGEAR_NO_COMB);
				}

				if (bits & EVT_ACT_HGEAR_COMB_ING)
				{
					StateMachine_SendEvent(&axis->sm_act, EVT_ACT_HGEAR_COMB_ING);
					EventGroup_Clear(axis->evt_act, EVT_ACT_HGEAR_COMB_ING);
				}

				if (bits & EVT_ACT_HGEAR_CANNEL)
				{
					StateMachine_SendEvent(&axis->sm_act, EVT_ACT_HGEAR_CANNEL);
					EventGroup_Clear(axis->evt_act, EVT_ACT_HGEAR_CANNEL);
				}

				if (bits & EVT_ACT_HGEAR_COMB_EXIT)
				{
					StateMachine_SendEvent(&axis->sm_act, EVT_ACT_HGEAR_COMB_EXIT);
					EventGroup_Clear(axis->evt_act, EVT_ACT_HGEAR_COMB_EXIT);
				}

				if (bits & EVT_ACT_COMBINE_FAIL)
				{
					StateMachine_SendEvent(&axis->sm_act, EVT_ACT_COMBINE_FAIL);
					EventGroup_Clear(axis->evt_act, EVT_ACT_COMBINE_FAIL);
				}

				if (bits & EVT_ACT_SEPARATE_FAIL)
				{
					StateMachine_SendEvent(&axis->sm_act, EVT_ACT_SEPARATE_FAIL);
					EventGroup_Clear(axis->evt_act, EVT_ACT_SEPARATE_FAIL);
				}

				if (bits & EVT_ACT_SEPARATE)
				{
					StateMachine_SendEvent(&axis->sm_act, EVT_ACT_SEPARATE);
					EventGroup_Clear(axis->evt_act, EVT_ACT_SEPARATE);
				}

				if (bits & EVT_ACT_COMBINE)
				{
					StateMachine_SendEvent(&axis->sm_act, EVT_ACT_COMBINE);
					EventGroup_Clear(axis->evt_act, EVT_ACT_COMBINE);
				}

				if (bits & EVT_ACT_COMBINE_TRY_1)
				{
					StateMachine_SendEvent(&axis->sm_act, EVT_ACT_COMBINE_TRY_1);
					EventGroup_Clear(axis->evt_act, EVT_ACT_COMBINE_TRY_1);
				}

				if (bits & EVT_ACT_COMBINE_TRY_2)
				{
					StateMachine_SendEvent(&axis->sm_act, EVT_ACT_COMBINE_TRY_2);
					EventGroup_Clear(axis->evt_act, EVT_ACT_COMBINE_TRY_2);
				}

				if (bits & EVT_ACT_COMBINE_BACK)
				{
					StateMachine_SendEvent(&axis->sm_act, EVT_ACT_COMBINE_BACK);
					EventGroup_Clear(axis->evt_act, EVT_ACT_COMBINE_BACK);
				}

				if (bits & EVT_ACT_COMBINE_SUCCESS)
				{
					StateMachine_SendEvent(&axis->sm_act, EVT_ACT_COMBINE_SUCCESS);
					EventGroup_Clear(axis->evt_act, EVT_ACT_COMBINE_SUCCESS);
				}

				if (bits & EVT_ACT_COMBINE_FAIL)
				{
					StateMachine_SendEvent(&axis->sm_act, EVT_ACT_COMBINE_FAIL);
					EventGroup_Clear(axis->evt_act, EVT_ACT_COMBINE_FAIL);
				}

				if (bits & EVT_ACT_SEPARATE_TRY_1)
				{
					StateMachine_SendEvent(&axis->sm_act, EVT_ACT_SEPARATE_TRY_1);
					EventGroup_Clear(axis->evt_act, EVT_ACT_SEPARATE_TRY_1);
				}

				if (bits & EVT_ACT_SEPARATE_TRY_2)
				{
					StateMachine_SendEvent(&axis->sm_act, EVT_ACT_SEPARATE_TRY_2);
					EventGroup_Clear(axis->evt_act, EVT_ACT_SEPARATE_TRY_2);
				}

				if (bits & EVT_ACT_SEPARATE_BACK)
				{
					StateMachine_SendEvent(&axis->sm_act, EVT_ACT_SEPARATE_BACK);
					EventGroup_Clear(axis->evt_act, EVT_ACT_SEPARATE_BACK);
				}

				if (bits & EVT_ACT_SEPARATE_SUCCESS)
				{
					StateMachine_SendEvent(&axis->sm_act, EVT_ACT_SEPARATE_SUCCESS);
					EventGroup_Clear(axis->evt_act, EVT_ACT_SEPARATE_SUCCESS);
				}

			}
		}
   }
}

/* 状态入口函数 */
static void act_enter_idle(void)              {
    //260611_RL_add:
    uint8_t i = mySystem.target_id;

	if ( i < MAX_AXIS_NUM)
        if (mySystem.axis[i].enable)
        {
          if (mySystem.axis[i].sm_act.cur_state == ACT_STATE_SEPARATE_SUCCESS ||
              mySystem.axis[i].sm_act.cur_state == ACT_STATE_SEPARATE_FAIL ||
              mySystem.axis[i].sm_act.cur_state == ACT_STATE_COMBINE_SUCCESS ||
              mySystem.axis[i].sm_act.cur_state == ACT_STATE_COMBINE_FAIL ||
              mySystem.axis[i].sm_act.cur_state == ACT_STATE_INIT ||
              mySystem.axis[i].sm_act.cur_state == ACT_STATE_IDLE  )
          {
             EventGroup_Send(mySystem.axis[i].motor.evt_mot, EVT_MOT_STOP);     //推杆停止

			  led_set_event(mySystem.axis[i].id, EVT_LEDx_OFF);
              //260709_add:增加推杆不同状态下led的状态
//              switch(mySystem.axis[i].sm_act.cur_state)
//              {
//                 case ACT_STATE_SEPARATE_SUCCESS:
//                 break;
//
//                 case ACT_STATE_COMBINE_SUCCESS:
//                 break;
//
//                 case ACT_STATE_SEPARATE_FAIL:
//                 case ACT_STATE_COMBINE_FAIL:
//                      //红灯闪烁三次
//                    led_set_event(mySystem.axis[i].id, (EVT_LEDx_RED_ON | EVT_LEDx_BLINK_LOOP | EVT_LEDx_FREQ_3xHZ_50 | EVT_LEDx_PRIORITY_4));
//                 break;
//
//                 case ACT_STATE_IDLE:
//					led_set_event(mySystem.axis[i].id,EVT_LEDx_OFF);
//                 break;
//
//                 default:
//                     break;
//
//
//			}
		}
	}
}

static void act_enter_moving(void)            {
	LOG_WARN("--act_enter_moving: starting----------------");

	uint8_t i = mySystem.target_id;

   if ( i < MAX_AXIS_NUM)
   {

      if (mySystem.axis[i].enable)
      {
         mySystem.axis[i].pos_ctrl.enable = true;
         mySystem.axis[i].spd_ctrl.enable = true;
         mySystem.axis[i].curr_ctrl.enable = true;

         PID_ResetOutput(&mySystem.axis[i].curr_ctrl);        //260722——add；每次进来先清空pid计算

		  //CF小推力机型不限力，限流值改到4000
		 if (52 == mySystem.eff_model || 53 == mySystem.eff_model || 181 == mySystem.eff_model || 0 == mySystem.eff_model)
		 {
			 PID_SetTarget(&mySystem.axis[i].curr_ctrl, 4000);
			 ctrl_set_over_current(i , 3900);
		 }else
		 {
			 PID_SetTarget(&mySystem.axis[i].curr_ctrl, 1500);    //在给电流环目标值
			 ctrl_set_over_current(i , 1400);
		 }

         if (mySystem.axis[i].sm_act.cur_state == ACT_STATE_COMBINE ||
            mySystem.axis[i].sm_act.cur_state == ACT_STATE_COMBINE_TRY_1 ||
            mySystem.axis[i].sm_act.cur_state == ACT_STATE_COMBINE_TRY_2 ||
            mySystem.axis[i].sm_act.cur_state == ACT_STATE_SEPARATE_BACK ||
            mySystem.axis[i].sm_act.cur_state == ACT_STATE_SEPARATE_BACK_1 ||
            mySystem.axis[i].sm_act.cur_state == ACT_STATE_SEPARATE_BACK_2 )
         {
			 LOG_WARN("mySystem.axis[%d].sm_act.cur_state == %d",i,mySystem.axis[i].sm_act.cur_state);

             //260723_add:增加位置判断
            if (fabs(mySystem.axis[i].pos_current - mySystem.axis[i].pos_combine ) <1.0)
            {
				EventGroup_Send(mySystem.axis[i].evt_act, EVT_ACT_COMBINE_SUCCESS);
                return;
            }

            if (mySystem.axis[i].pos_current > mySystem.axis[i].pos_combine )
            {
               mySystem.axis[i].pos_ctrl.out_nor = -1.0f;
            }else{
               mySystem.axis[i].pos_ctrl.out_nor = 1.0f;
            }

            mySystem.axis[i].pos_ctrl.set = mySystem.axis[i].pos_combine;

            // 结合方向
            if (mySystem.axis[i].pos_ctrl.out_nor == 1)  //260723——add:增加当前位置和目标位置判断
            {
				LOG_WARN("mySystem.axis[%d].motor.evt_mot, EVT_MOT_FORWARD",i);
               EventGroup_Send(mySystem.axis[i].motor.evt_mot, EVT_MOT_FORWARD);
            }else if(mySystem.axis[i].pos_ctrl.out_nor == -1)
            {
				LOG_WARN("mySystem.axis[%d].motor.evt_mot, EVT_MOT_REVERSE",i);
               EventGroup_Send(mySystem.axis[i].motor.evt_mot, EVT_MOT_REVERSE);
            }
         }else if (mySystem.axis[i].sm_act.cur_state == ACT_STATE_SEPARATE ||
                  mySystem.axis[i].sm_act.cur_state == ACT_STATE_SEPARATE_TRY_1 ||
                  mySystem.axis[i].sm_act.cur_state == ACT_STATE_SEPARATE_TRY_2 ||
                  mySystem.axis[i].sm_act.cur_state == ACT_STATE_COMBINE_BACK   ||
                  mySystem.axis[i].sm_act.cur_state == ACT_STATE_COMBINE_BACK_1 ||
                  mySystem.axis[i].sm_act.cur_state == ACT_STATE_COMBINE_BACK_2 )
         {
			 LOG_WARN("mySystem.axis[%d].sm_act.cur_state == %d",i,mySystem.axis[i].sm_act.cur_state);
            //260723_add:增加位置判断
            if (fabs(mySystem.axis[i].pos_current - mySystem.axis[i].pos_separate ) <= 1.0)
            {
				EventGroup_Send(mySystem.axis[i].evt_act, EVT_ACT_SEPARATE_SUCCESS);
                return;
            }

            if (mySystem.axis[i].pos_current > mySystem.axis[i].pos_separate )
            {
               mySystem.axis[i].pos_ctrl.out_nor = -1.0f;
            }else{
               mySystem.axis[i].pos_ctrl.out_nor = 1.0f;
            }

            mySystem.axis[i].pos_ctrl.set = mySystem.axis[i].pos_separate;
            // 分离方向
            if (mySystem.axis[i].pos_ctrl.out_nor == 1)    //260723——add:增加当前位置和目标位置判断
            {
				LOG_WARN("mySystem.axis[%d].motor.evt_mot, EVT_MOT_FORWARD",i);
               EventGroup_Send(mySystem.axis[i].motor.evt_mot, EVT_MOT_FORWARD);
            }else if (mySystem.axis[i].pos_ctrl.out_nor == -1)
            {
				LOG_WARN("mySystem.axis[%d].motor.evt_mot, EVT_MOT_REVERSE",i);
               EventGroup_Send(mySystem.axis[i].motor.evt_mot, EVT_MOT_REVERSE);
            }
         }

			led_set_event(mySystem.axis[i].id, EVT_LEDx_OFF);
//        //260709_add:增加推杆不同状态下led的状态——正在结合/正在分离 都是绿灯慢闪
//        switch(mySystem.axis[i].sm_act.cur_state)
//        {
//            case ACT_STATE_COMBINE:
//            case ACT_STATE_COMBINE_TRY_1:
//            case ACT_STATE_COMBINE_TRY_2:
//            case ACT_STATE_SEPARATE_BACK:
//            case ACT_STATE_SEPARATE_BACK_1:
//            case ACT_STATE_SEPARATE:
//            case ACT_STATE_SEPARATE_TRY_1:
//            case ACT_STATE_SEPARATE_TRY_2:
//            case ACT_STATE_COMBINE_BACK:
//            case ACT_STATE_COMBINE_BACK_1:
//			//绿灯闪
//				led_set_event(mySystem.axis[i].id, (EVT_LEDx_FREQ_2HZ_50 | EVT_LEDx_BLINK_LOOP | EVT_LEDx_GREEN_ON | EVT_LEDx_PRIORITY_5));
//             break;
//            default:
//             break;
//        }

      }
   }
//   LOG_WARN("--act_enter_moving: end----------------");
}

static void act_enter_combine(void){
}
static void act_enter_separate(void)          {
}
static void act_enter_hold(void)              {
	uint8_t i = mySystem.target_id;

   if ( i < MAX_AXIS_NUM)
   {
        if (mySystem.axis[i].enable)
        {
      EventGroup_Send(mySystem.axis[i].motor.evt_mot, EVT_MOT_STOP);

      mySystem.axis[i].pos_ctrl.enable = false;
      mySystem.axis[i].spd_ctrl.enable = false;
      mySystem.axis[i].curr_ctrl.enable = false;

            //260709_add:增加入口函数中关于led灯的控制
            switch(mySystem.axis[i].sm_act.cur_state)
            {
                case ACT_STATE_OVERSPEED_ALARM:
                case ACT_STATE_H_GEAR_COMB_5S:

					if (i == 0)
					{
						SoftTimer_Start(act_htim_m1_fault, SOFT_TIMER_MODE_ONCE, 5000);
					}else if (i == 1)
					{
						SoftTimer_Start(act_htim_m2_fault, SOFT_TIMER_MODE_ONCE, 5000);
					}
				 break;

                case ACT_STATE_H_GEAR_COMB_ED:
                break;
                default:
                break;
            }

        }
   }
}

static void act_exit(void)
{
	for (uint8_t i = 0; i < MAX_AXIS_NUM; i++)
   {
        if (mySystem.axis[i].enable)
        {
            switch(mySystem.axis[i].sm_act.cur_state)
            {
                case ACT_STATE_OVERSPEED_ALARM:
                case ACT_STATE_H_GEAR_COMB_5S:
				case ACT_STATE_H_GEAR_COMB_ED:

                    led_set_event(mySystem.axis[i].id, EVT_LEDx_OFF);
                break;
                default:
                break;
            }

        }
   }
}

uint16_t g_test = 0;
uint16_t g_test_ = 0;
/*
260611_RL_add:
堵转后调用函数*/
void act_babk_handler(void)
{
    //2.1给推杆发送事件,根据推杆当前状态发送
    //EventGroup_Send(mySystem.axis[i].evt_act, EVT_ACT_SEPARATE);
    //2.2 推杆回退需要计时1.2s
    //2.3 计时1.2s之后，发尝试事件
    //2.4 (几次try中接收到位置环的成功事件；否则报失败)
    //260713_add:增加:回退不能超过 回退对应方向 分离点/结合点 的限制

   static uint32_t s_BackStartTime[MAX_AXIS_NUM] = {0};

   for (uint8_t i = 0; i < MAX_AXIS_NUM; i++)
   {
       if(mySystem.axis[i].act_stall_flag)
       {
          //PID_ResetOutput(&mySystem.axis[i].curr_ctrl);        //260710——add；清空pid计算
      if (mySystem.axis[i].sm_act.cur_state == ACT_STATE_SEPARATE ||
          mySystem.axis[i].sm_act.cur_state == ACT_STATE_SEPARATE_TRY_1 ||
              mySystem.axis[i].sm_act.cur_state == ACT_STATE_SEPARATE_TRY_2 )//每次回退都要有1.2s的限时。
          {
			                    //发送分离遇阻事件 EVT_ACT_SEPARATE_BACK
                  EventGroup_Send(mySystem.axis[i].evt_act, EVT_ACT_SEPARATE_BACK);
                  //开始计时
                  s_BackStartTime[i] = SysTick_GetTick();

//              if(mySystem.axis[i].sm_act.cur_state == ACT_STATE_SEPARATE_TRY_2)
//              {
//                //发送堵转，分离失败,不用计时
//                EventGroup_Send(mySystem.axis[i].evt_act, EVT_ACT_SEPARATE_BACK);
//                //清除堵转标志
//                //*MotorBackFlag = 0;
//                mySystem.axis[i].act_stall_flag = false;
//              }
//              else
//              {
//                  //发送分离遇阻事件 EVT_ACT_SEPARATE_BACK
//                  EventGroup_Send(mySystem.axis[i].evt_act, EVT_ACT_SEPARATE_BACK);
//                  //开始计时
//                  s_BackStartTime[i] = SysTick_GetTick();
//                //SoftTimer_Start(Act_htim_m1_enter, SOFT_TIMER_MODE_ONCE, ACT_BACK_MS);
//              }
          }
          else if(mySystem.axis[i].sm_act.cur_state == ACT_STATE_COMBINE   ||
                  mySystem.axis[i].sm_act.cur_state == ACT_STATE_COMBINE_TRY_1 ||
                  mySystem.axis[i].sm_act.cur_state == ACT_STATE_COMBINE_TRY_2 )      //每次回退都要有1.2s的限时。
          {
			                  //发送结合遇阻事件 EVT_ACT_COMBINE_BACK/失败
                EventGroup_Send(mySystem.axis[i].evt_act, EVT_ACT_COMBINE_BACK);
                //开始计时
                s_BackStartTime[i] = SysTick_GetTick();
//            if(mySystem.axis[i].sm_act.cur_state == ACT_STATE_COMBINE_TRY_2)
//            {
//                //发送堵转事件，直接跳到失败，不需要计时
//                EventGroup_Send(mySystem.axis[i].evt_act, EVT_ACT_COMBINE_BACK);
//                //清除堵转标志
//                //*MotorBackFlag = 0;
//                mySystem.axis[i].act_stall_flag = false;
//            }
//            else
//            {
//                //发送结合遇阻事件 EVT_ACT_COMBINE_BACK/失败
//                EventGroup_Send(mySystem.axis[i].evt_act, EVT_ACT_COMBINE_BACK);
//                //开始计时
//                s_BackStartTime[i] = SysTick_GetTick();
//                SoftTimer_Start(Act_htim_m1_enter, SOFT_TIMER_MODE_ONCE, ACT_BACK_MS);
//            }
        }
    }

      if (mySystem.axis[i].sm_act.cur_state == ACT_STATE_SEPARATE_BACK ||
          mySystem.axis[i].sm_act.cur_state == ACT_STATE_SEPARATE_BACK_1 ||
          mySystem.axis[i].sm_act.cur_state == ACT_STATE_SEPARATE_BACK_2 )
      {
        ////超过1.2 ,计时到了以后发再次尝试分离
        //260713_add:增加回退不能超过对应方向 结合点 的限制
        uint32_t NowTime = SysTick_GetTick();
        if(((NowTime - s_BackStartTime[i]) > ACT_BACK_MS)  ||
            ((mySystem.axis[i].pos_ctrl.fdb < mySystem.axis[i].pos_combine + mySystem.axis[i].pos_ctrl.stall_thr) && mySystem.axis[i].dir == ACT_DIR_COMBINE_IS_MOVE_IN) ||
            ((mySystem.axis[i].pos_ctrl.fdb > mySystem.axis[i].pos_combine - mySystem.axis[i].pos_ctrl.stall_thr ) && mySystem.axis[i].dir == ACT_DIR_COMBINE_IS_MOVE_OUT) )
        {
//            if(mySystem.axis[i].sm_act.cur_state == ACT_STATE_SEPARATE_BACK_2 )
//            {
//                //最后一次回退1.2s后直接停止
//            }
//            else{
            //根据当前状态发再次分离
                EventGroup_Send(mySystem.axis[i].evt_act, EVT_ACT_SEPARATE);
//            }
            //清除堵转标志
            //*MotorBackFlag = 0;
             mySystem.axis[i].act_stall_flag = false;
        }
      }
      else if(mySystem.axis[i].sm_act.cur_state == ACT_STATE_COMBINE_BACK   ||
              mySystem.axis[i].sm_act.cur_state == ACT_STATE_COMBINE_BACK_1 ||
              mySystem.axis[i].sm_act.cur_state == ACT_STATE_COMBINE_BACK_2 )
      {
		  g_test++;
        ////超过1.2 ,计时到了以后发再次尝试结合
        //260713_add:增加回退不能超过对应方向 分离/结合点 的限制
        uint32_t NowTime = SysTick_GetTick();
        if(((NowTime - s_BackStartTime[i]) > ACT_BACK_MS)   ||
            ((mySystem.axis[i].pos_ctrl.fdb < mySystem.axis[i].pos_separate + mySystem.axis[i].pos_ctrl.stall_thr) && mySystem.axis[i].dir == ACT_DIR_COMBINE_IS_MOVE_OUT) ||
            ((mySystem.axis[i].pos_ctrl.fdb > mySystem.axis[i].pos_separate - mySystem.axis[i].pos_ctrl.stall_thr ) && mySystem.axis[i].dir == ACT_DIR_COMBINE_IS_MOVE_IN) )
        {
			g_test_ ++;
            //根据当前状态发再次分离
            EventGroup_Send(mySystem.axis[i].evt_act, EVT_ACT_COMBINE);
            //清除堵转标志
            //*MotorBackFlag = 0;
            mySystem.axis[i].act_stall_flag = false;
        }
      }
  }
}

static void Act_TimerCb_M1Enter(SoftTimer_Handle_t htimer)
{
//    //根据当前状态发再次分离
//    EventGroup_Send(mySystem.axis[0].evt_act, EVT_ACT_COMBINE);
////    EventGroup_Send(mySystem.axis[1].evt_act, EVT_ACT_COMBINE);
//    //            }
//    //清除堵转标志
//    g_ActStallFlag = 0;

}
static void Act_TimerCb_M1Exit(SoftTimer_Handle_t htimer)
{

}
static void Act_TimerCb_M2Enter(SoftTimer_Handle_t htimer)
{

}
static void Act_TimerCb_M2Exit(SoftTimer_Handle_t htimer)
{

}

static void Act_TimerCb_m1_fault(SoftTimer_Handle_t htimer)
{
    if (ACT_STATE_OVERSPEED_ALARM == mySystem.axis[0].sm_act.cur_state)
	{
		EventGroup_Send(mySystem.axis[0].evt_act, EVT_ACT_OVERSPEED_ALARM_DISABLE);
	}

	if (ACT_STATE_H_GEAR_COMB_5S == mySystem.axis[0].sm_act.cur_state)
	{
		EventGroup_Send(mySystem.axis[0].evt_act, EVT_ACT_HGEAR_COMB_EXIT);
	}
}

static void Act_TimerCb_m2_fault(SoftTimer_Handle_t htimer)
{
    if (ACT_STATE_OVERSPEED_ALARM == mySystem.axis[1].sm_act.cur_state)
	{
		EventGroup_Send(mySystem.axis[1].evt_act, EVT_ACT_OVERSPEED_ALARM_DISABLE);
	}

	if (ACT_STATE_H_GEAR_COMB_5S == mySystem.axis[1].sm_act.cur_state)
	{
		EventGroup_Send(mySystem.axis[1].evt_act, EVT_ACT_HGEAR_COMB_EXIT);
	}
}

/**********************26.7.2 zjw 标定状态机*******************************/

//状态跳转表
static const StateJumpTable_t calib_jump[] = {
	{CALIB_STATE_INIT,				EVT_CALIB_INIT_DONE,			CALIB_STATE_IDLE},

	{CALIB_STATE_IDLE,				EVT_CALIB_INTO_MANU_ING,		CALIB_STATE_INTO_MANU_ING},
	{CALIB_STATE_IDLE,				EVT_CALIB_AUTO_CALIB_SEP,		CALIB_STATE_AUTO_CALIB_SEP},

	{CALIB_STATE_INTO_MANU_ING,		EVT_CALIB_INTO_MANU,			CALIB_STATE_INTO_MANU_ED},
	{CALIB_STATE_INTO_MANU_ING,		EVT_CALIB_INTO_MANU_CANCEL,		CALIB_STATE_IDLE},
	{CALIB_STATE_INTO_MANU_ING,		EVT_CALIB_EST,					CALIB_STATE_IDLE},

	{CALIB_STATE_INTO_MANU_ED,		EVT_CALIB_INTO_MANU_ED,			CALIB_STATE_MANU_IDLE},
	{CALIB_STATE_INTO_MANU_ED,		EVT_CALIB_EST,					CALIB_STATE_IDLE},

	{CALIB_STATE_MANU_IDLE,			EVT_CALIB_MANU_OUT,				CALIB_STATE_MANU_OUT},
	{CALIB_STATE_MANU_IDLE,			EVT_CALIB_MANU_IN,				CALIB_STATE_MANU_IN},
	{CALIB_STATE_MANU_IDLE,			EVT_CALIB_EXIT_MANU_ING,		CALIB_STATE_EXIT_MANU_ING},
	{CALIB_STATE_MANU_IDLE,			EVT_CALIB_DONE,					CALIB_STATE_IDLE},
	{CALIB_STATE_MANU_IDLE,			EVT_CALIB_AUTO_COMB,			CALIB_STATE_IDLE},
	{CALIB_STATE_MANU_IDLE,			EVT_CALIB_AUTO_SEP,				CALIB_STATE_IDLE},
	{CALIB_STATE_MANU_IDLE,			EVT_CALIB_AUTO_CALIB_SEP,		CALIB_STATE_AUTO_CALIB_SEP},
	{CALIB_STATE_MANU_IDLE,			EVT_CALIB_EXIT,					CALIB_STATE_IDLE},
	{CALIB_STATE_MANU_IDLE,			EVT_CALIB_EST,					CALIB_STATE_IDLE},

	{CALIB_STATE_MANU_OUT,			EVT_CALIB_MANU_STOP,			CALIB_STATE_MANU_IDLE},
	{CALIB_STATE_MANU_OUT,			EVT_CALIB_AUTO_COMB,			CALIB_STATE_IDLE},
	{CALIB_STATE_MANU_OUT,			EVT_CALIB_AUTO_SEP,				CALIB_STATE_IDLE},
	{CALIB_STATE_MANU_OUT,			EVT_CALIB_AUTO_CALIB_SEP,		CALIB_STATE_AUTO_CALIB_SEP},
	{CALIB_STATE_MANU_OUT,			EVT_CALIB_DONE,					CALIB_STATE_IDLE},
	{CALIB_STATE_MANU_OUT,			EVT_CALIB_EXIT_MANU_ING,		CALIB_STATE_EXIT_MANU_ING},
	{CALIB_STATE_MANU_OUT,			EVT_CALIB_EXIT,					CALIB_STATE_IDLE},
	{CALIB_STATE_MANU_OUT,			EVT_CALIB_EST,					CALIB_STATE_IDLE},

	{CALIB_STATE_MANU_IN,			EVT_CALIB_MANU_STOP,			CALIB_STATE_MANU_IDLE},
	{CALIB_STATE_MANU_IN,			EVT_CALIB_AUTO_COMB,			CALIB_STATE_IDLE},
	{CALIB_STATE_MANU_IN,			EVT_CALIB_AUTO_SEP,				CALIB_STATE_IDLE},
	{CALIB_STATE_MANU_IN,			EVT_CALIB_AUTO_CALIB_SEP,		CALIB_STATE_AUTO_CALIB_SEP},
	{CALIB_STATE_MANU_IN,			EVT_CALIB_DONE,					CALIB_STATE_IDLE},
	{CALIB_STATE_MANU_IN,			EVT_CALIB_EXIT_MANU_ING,		CALIB_STATE_EXIT_MANU_ING},
	{CALIB_STATE_MANU_IN,			EVT_CALIB_EXIT,					CALIB_STATE_IDLE},
	{CALIB_STATE_MANU_IN,			EVT_CALIB_EST,					CALIB_STATE_IDLE},

	{CALIB_STATE_EXIT_MANU_ING,		EVT_CALIB_EXIT_MANU_CANCEL,		CALIB_STATE_MANU_IDLE},
	{CALIB_STATE_EXIT_MANU_ING,		EVT_CALIB_DONE,					CALIB_STATE_IDLE},
	{CALIB_STATE_EXIT_MANU_ING,		EVT_CALIB_EXIT,					CALIB_STATE_IDLE},
	{CALIB_STATE_EXIT_MANU_ING,		EVT_CALIB_EST,					CALIB_STATE_IDLE},

	{CALIB_STATE_AUTO_CALIB_SEP,	EVT_CALIB_AUTO_CALIB_BACK,		CALIB_STATE_AUTO_CALIB_BACK},
	{CALIB_STATE_AUTO_CALIB_SEP,	EVT_CALIB_EST,					CALIB_STATE_IDLE},

	{CALIB_STATE_AUTO_CALIB_BACK,	EVT_CALIB_AUTO_DONE,			CALIB_STATE_IDLE},
	{CALIB_STATE_AUTO_CALIB_BACK,	EVT_CALIB_EST,					CALIB_STATE_IDLE},
};

/* 状态入口函数声明 */
static void calib_enter_init(void);
static void calib_enter_idle(void);
static void calib_enter_run(void);

/**
 * @brief 标定状态入口函数表
 */
static const StateFuncTable_t calib_func[] = {
   {CALIB_STATE_INIT,				calib_enter_init,	0},
   {CALIB_STATE_IDLE,				calib_enter_idle,	0},

   {CALIB_STATE_INTO_MANU_ING,		calib_enter_idle,	0},
   {CALIB_STATE_INTO_MANU_ED,		calib_enter_run,	0},
   {CALIB_STATE_MANU_IDLE,			calib_enter_run,	0},
   {CALIB_STATE_MANU_OUT,			calib_enter_run,	0},
   {CALIB_STATE_MANU_IN,			calib_enter_run,	0},
   {CALIB_STATE_EXIT_MANU_ING,		calib_enter_run,	0},
   {CALIB_STATE_AUTO_CALIB_SEP,		calib_enter_run,	0},
   {CALIB_STATE_AUTO_CALIB_BACK,	calib_enter_run,	0},
};

/**
 * @brief 推杆状态机 初始化
 * @note
 */
void Calib_State_Init(StateMachine_t *sm)
{
    StateMachine_t *sm_temp = sm;
    sm_temp->jump_table = calib_jump;
    sm_temp->jump_table_size = sizeof(calib_jump)/sizeof(StateJumpTable_t);
    sm_temp->func_table = calib_func;
    sm_temp->func_table_size = sizeof(calib_func)/sizeof(StateFuncTable_t);
    sm_temp->init_state = ACT_STATE_IDLE;

    StateMachine_Init(sm_temp);
}
/**
 * @brief 推杆状态机运行
 * @param  axis: 轴对象
 * @note   处理事件，下发电动机指令，系统状态机会不断调度
 */
void Calib_State_Task(Axis_t *axis)
{
	EventBits_t bits = EventGroup_Get(axis->evt_calib);

	if (bits & EVT_CALIB_INIT_DONE)
	{
		StateMachine_SendEvent(&axis->sm_calib, EVT_CALIB_INIT_DONE);
		EventGroup_Clear(axis->evt_calib, EVT_CALIB_INIT_DONE);
	}

	if (bits & EVT_CALIB_ENABLE)
	{
		StateMachine_SendEvent(&axis->sm_calib, EVT_CALIB_ENABLE);
		EventGroup_Clear(axis->evt_calib, EVT_CALIB_ENABLE);
	}

	if (bits & EVT_CALIB_DISABLE)
	{
		StateMachine_SendEvent(&axis->sm_calib, EVT_CALIB_DISABLE);
		EventGroup_Clear(axis->evt_calib, EVT_CALIB_DISABLE);
	}

	if (bits & EVT_CALIB_EST)
	{
		StateMachine_SendEvent(&axis->sm_calib, EVT_CALIB_EST);
		EventGroup_Clear(axis->evt_calib, EVT_CALIB_EST);
	}

	if (bits & EVT_CALIB_INTO_MANU_ING)
	{
		StateMachine_SendEvent(&axis->sm_calib, EVT_CALIB_INTO_MANU_ING);
		EventGroup_Clear(axis->evt_calib, EVT_CALIB_INTO_MANU_ING);
	}

	if (bits & EVT_CALIB_INTO_MANU_CANCEL)
	{
		StateMachine_SendEvent(&axis->sm_calib, EVT_CALIB_INTO_MANU_CANCEL);
		EventGroup_Clear(axis->evt_calib, EVT_CALIB_INTO_MANU_CANCEL);
	}

	if (bits & EVT_CALIB_INTO_MANU)
	{
		StateMachine_SendEvent(&axis->sm_calib, EVT_CALIB_INTO_MANU);
		EventGroup_Clear(axis->evt_calib, EVT_CALIB_INTO_MANU);
	}

	if (bits & EVT_CALIB_MANU_OUT)
	{
		StateMachine_SendEvent(&axis->sm_calib, EVT_CALIB_MANU_OUT);
		EventGroup_Clear(axis->evt_calib, EVT_CALIB_MANU_OUT);
	}

	if (bits & EVT_CALIB_MANU_IN)
	{
		StateMachine_SendEvent(&axis->sm_calib, EVT_CALIB_MANU_IN);
		EventGroup_Clear(axis->evt_calib, EVT_CALIB_MANU_IN);
	}

	if (bits & EVT_CALIB_MANU_STOP)
	{
		StateMachine_SendEvent(&axis->sm_calib, EVT_CALIB_MANU_STOP);
		EventGroup_Clear(axis->evt_calib, EVT_CALIB_MANU_STOP);
	}

	if (bits & EVT_CALIB_EXIT_MANU_ING)
	{
		StateMachine_SendEvent(&axis->sm_calib, EVT_CALIB_EXIT_MANU_ING);
		EventGroup_Clear(axis->evt_calib, EVT_CALIB_EXIT_MANU_ING);
	}

	if (bits & EVT_CALIB_EXIT_MANU_CANCEL)
	{
		StateMachine_SendEvent(&axis->sm_calib, EVT_CALIB_EXIT_MANU_CANCEL);
		EventGroup_Clear(axis->evt_calib, EVT_CALIB_EXIT_MANU_CANCEL);
	}

	if (bits & EVT_CALIB_EXIT)
	{
		StateMachine_SendEvent(&axis->sm_calib, EVT_CALIB_EXIT);
		EventGroup_Clear(axis->evt_calib, EVT_CALIB_EXIT);
	}

	if (bits & EVT_CALIB_AUTO_CALIB_SEP)
	{
		StateMachine_SendEvent(&axis->sm_calib, EVT_CALIB_AUTO_CALIB_SEP);
		EventGroup_Clear(axis->evt_calib, EVT_CALIB_AUTO_CALIB_SEP);
	}

	if (bits & EVT_CALIB_AUTO_CALIB_BACK)
	{
		StateMachine_SendEvent(&axis->sm_calib, EVT_CALIB_AUTO_CALIB_BACK);
		EventGroup_Clear(axis->evt_calib, EVT_CALIB_AUTO_CALIB_BACK);
	}

	if (bits & EVT_CALIB_AUTO_DONE)
	{
		StateMachine_SendEvent(&axis->sm_calib, EVT_CALIB_AUTO_DONE);
		EventGroup_Clear(axis->evt_calib, EVT_CALIB_AUTO_DONE);
	}

	if (bits & EVT_CALIB_AUTO_COMB)
	{
		StateMachine_SendEvent(&axis->sm_calib, EVT_CALIB_AUTO_COMB);
		EventGroup_Clear(axis->evt_calib, EVT_CALIB_AUTO_COMB);
	}

	if (bits & EVT_CALIB_AUTO_SEP)
	{
		StateMachine_SendEvent(&axis->sm_calib, EVT_CALIB_AUTO_SEP);
		EventGroup_Clear(axis->evt_calib, EVT_CALIB_AUTO_SEP);
	}

	if (bits & EVT_CALIB_DONE)
	{
		StateMachine_SendEvent(&axis->sm_calib, EVT_CALIB_DONE);
		EventGroup_Clear(axis->evt_calib, EVT_CALIB_DONE);
	}

	if (bits & EVT_CALIB_INTO_MANU_ED)
	{
		StateMachine_SendEvent(&axis->sm_calib, EVT_CALIB_INTO_MANU_ED);
		EventGroup_Clear(axis->evt_calib, EVT_CALIB_INTO_MANU_ED);
	}

	if (bits & EVT_CALIB_EST)
	{
		StateMachine_SendEvent(&axis->sm_calib, EVT_CALIB_EST);
		EventGroup_Clear(axis->evt_calib, EVT_CALIB_EST);
	}

}

static void calib_enter_init(void)
{
	//跳转
	EventGroup_Send(mySystem.axis[0].evt_calib, EVT_CALIB_INIT_DONE);
	EventGroup_Send(mySystem.axis[1].evt_calib, EVT_CALIB_INIT_DONE);
}

// 标定状态机空闲-->指未进行标定操作，系统按照正常工作模式运行
static void calib_enter_idle(void)
{
	mySystem.mode = SYS_MODE_NORMAL;

	uint8_t j = mySystem.target_id;

	if ( j < MAX_AXIS_NUM)
	{
		StateMachine_t *p_sm_calib = &mySystem.axis[j].sm_calib;

		mySystem.axis[j].mode = ACT_MODE_NORMAL;

		switch(p_sm_calib->cur_state)
		{
			case CALIB_STATE_IDLE:

				led_set_event(j, EVT_LEDx_OFF);	//灭，清除优先级
				if (EVT_CALIB_DONE == mySystem.axis[j].evt_calib->event_bits ||
					EVT_CALIB_AUTO_DONE == mySystem.axis[j].evt_calib->event_bits )//需要判断是否保存信息的情况
				{
					//推杆得停下 -- 这里应该有时间间隔  待规划
					EventGroup_Send(mySystem.axis[j].motor.evt_mot, EVT_MOT_STOP);
					model_pos_upd(j);

				}
				else if (EVT_CALIB_EXIT == mySystem.axis[j].evt_calib->event_bits)
				{
					EventGroup_Send(mySystem.axis[j].motor.evt_mot, EVT_MOT_STOP);
				}
				else if (EVT_CALIB_INTO_MANU_CANCEL == mySystem.axis[j].evt_calib->event_bits)//手动标定进入时候取消了
				{
					//只有灯变--进入这个状态之前系统模式还是正常，所以灯根据推杆状态机状态更新
				}
				else if (EVT_CALIB_AUTO_COMB == mySystem.axis[j].evt_calib->event_bits)
				{
					//转发命令
					key_state_reset();
					EventGroup_Send(mySystem.axis[j].evt_act, EVT_ACT_COMBINE);

				}
				else if (EVT_CALIB_AUTO_SEP == mySystem.axis[j].evt_calib->event_bits)
				{
					//转发命令
					key_state_reset();
					EventGroup_Send(mySystem.axis[j].evt_act, EVT_ACT_SEPARATE);
				}

			break;

			case CALIB_STATE_INTO_MANU_ING://手动标定待确认状态（这个要稍微犹豫一下放哪个函数）
				led_set_event(j, (EVT_LEDx_FREQ_10HZ_50 |  EVT_LEDx_BLINK_LOOP | EVT_LEDx_RED_YELLOW_ON));

				mySystem.manu_calib_motor_idx = j;

			break;
		}
	}
}

static void calib_enter_run(void)
{
	if (0 == mySystem.manu_calib_motor_idx)
	{
		mySystem.mode = SYS_MODE_CALIB_MODE_1;
	}else if (1 == mySystem.manu_calib_motor_idx)
	{
		mySystem.mode = SYS_MODE_CALIB_MODE_2;
	}

	uint8_t j = mySystem.target_id;

	if ( j < MAX_AXIS_NUM)
	{
		StateMachine_t *p_sm_calib = &mySystem.axis[j].sm_calib;

		mySystem.axis[j].mode = ACT_MODE_CALIB_MODE;

		EventGroup_Send(mySystem.axis[j].evt_act, EVT_ACT_RESET);

		switch(p_sm_calib->cur_state)
		{
			case CALIB_STATE_INTO_MANU_ED:
				//黄灯常亮
				led_set_event(j, EVT_LEDx_YELLOW_ON);
			break;

			case CALIB_STATE_MANU_IDLE://手动标定的空闲状态
				//推杆得停下
				EventGroup_Send(mySystem.axis[j].motor.evt_mot, EVT_MOT_STOP);
				//黄灯闪烁
				led_set_event(j, (EVT_LEDx_FREQ_2HZ_50 |  EVT_LEDx_BLINK_LOOP | EVT_LEDx_YELLOW_ON));
				//机型按系统变量保存的来

			break;

			case CALIB_STATE_MANU_OUT:

				mySystem.axis[j].curr_ctrl.enable = true;
				mySystem.axis[j].pos_ctrl.enable = true;
				if (52 == mySystem.eff_model || 53 == mySystem.eff_model || 181 == mySystem.eff_model || 0 == mySystem.eff_model)
				{
					PID_SetTarget(&mySystem.axis[j].curr_ctrl, 4000);
					ctrl_set_over_current(j , 3900);
				}else
				{
					PID_SetTarget(&mySystem.axis[j].curr_ctrl, 1500);    //在给电流环目标值
					ctrl_set_over_current(j , 1400);
				}
				PID_SetTarget(&mySystem.axis[j].pos_ctrl, 100);
				//控制推杆伸出
				EventGroup_Send(mySystem.axis[j].motor.evt_mot, EVT_MOT_FORWARD);
				//绿灯闪
				led_set_event(j, (EVT_LEDx_FREQ_2HZ_50 | EVT_LEDx_BLINK_LOOP | EVT_LEDx_GREEN_ON));

			break;

			case CALIB_STATE_MANU_IN:

				mySystem.axis[j].curr_ctrl.enable = true;
				mySystem.axis[j].pos_ctrl.enable = true;
				if (52 == mySystem.eff_model || 53 == mySystem.eff_model || 181 == mySystem.eff_model || 0 == mySystem.eff_model)
				{
					PID_SetTarget(&mySystem.axis[j].curr_ctrl, 4000);
					ctrl_set_over_current(j , 3900);
				}else
				{
					PID_SetTarget(&mySystem.axis[j].curr_ctrl, 1500);    //在给电流环目标值
					ctrl_set_over_current(j , 1400);
				}
				PID_SetTarget(&mySystem.axis[j].pos_ctrl, 0);
				//控制推杆缩回
				EventGroup_Send(mySystem.axis[j].motor.evt_mot, EVT_MOT_REVERSE);
				//绿灯闪
				led_set_event(j, (EVT_LEDx_FREQ_2HZ_50 | EVT_LEDx_BLINK_LOOP | EVT_LEDx_GREEN_ON));

			break;

			case CALIB_STATE_EXIT_MANU_ING:
				//准备退出了 -- 俩按钮一起按
				EventGroup_Send(mySystem.axis[j].motor.evt_mot, EVT_MOT_STOP);
				//黄灯闪烁
				led_set_event(j, (EVT_LEDx_FREQ_2HZ_50 |  EVT_LEDx_BLINK_LOOP | EVT_LEDx_YELLOW_ON));
			break;

			case CALIB_STATE_AUTO_CALIB_SEP:
				mySystem.mode = SYS_MODE_AUTO_CALIB;
				//收到一键自动标定的命令--像分离方向运行，位置环的位置要写大点，这个要一直运行到堵转

				mySystem.axis[j].curr_ctrl.enable = true;
				mySystem.axis[j].pos_ctrl.enable = true;
				if (52 == mySystem.eff_model || 53 == mySystem.eff_model || 181 == mySystem.eff_model || 0 == mySystem.eff_model)
				{
					PID_SetTarget(&mySystem.axis[j].curr_ctrl, 4000);
					ctrl_set_over_current(j , 3900);
				}else
				{
					PID_SetTarget(&mySystem.axis[j].curr_ctrl, 1500);    //在给电流环目标值
					ctrl_set_over_current(j , 1400);
				}
				PID_SetTarget(&mySystem.axis[j].pos_ctrl, 100);

				if (mySystem.axis[j].dir == ACT_DIR_COMBINE_IS_MOVE_OUT)
				{
					EventGroup_Send(mySystem.axis[j].motor.evt_mot, EVT_MOT_REVERSE);
				}else if (mySystem.axis[j].dir == ACT_DIR_COMBINE_IS_MOVE_IN)
				{
					EventGroup_Send(mySystem.axis[j].motor.evt_mot, EVT_MOT_FORWARD);
				}

				//绿灯闪
				led_set_event(j, (EVT_LEDx_FREQ_2HZ_50 | EVT_LEDx_BLINK_LOOP | EVT_LEDx_GREEN_ON));
			break;

			case CALIB_STATE_AUTO_CALIB_BACK:

				mySystem.axis[j].curr_ctrl.enable = true;
				mySystem.axis[j].pos_ctrl.enable = true;
				if (52 == mySystem.eff_model || 53 == mySystem.eff_model || 181 == mySystem.eff_model || 0 == mySystem.eff_model)
				{
				 PID_SetTarget(&mySystem.axis[j].curr_ctrl, 4000);
				}else
				{
				 PID_SetTarget(&mySystem.axis[j].curr_ctrl, 1500);    //在给电流环目标值
				}
				//堵转之后收到位置环的事件会进入这个状态
				if (mySystem.axis[j].dir == ACT_DIR_COMBINE_IS_MOVE_OUT)
				{
					PID_SetTarget(&mySystem.axis[j].pos_ctrl, (mySystem.axis[j].pos_current + 2));
					//位置环目标位置为当前位置+2
					EventGroup_Send(mySystem.axis[j].motor.evt_mot, EVT_MOT_FORWARD);
				}else if (mySystem.axis[j].dir == ACT_DIR_COMBINE_IS_MOVE_IN)
				{
					PID_SetTarget(&mySystem.axis[j].pos_ctrl, (mySystem.axis[j].pos_current - 2));
					//位置环目标位置为当前位置-2
					EventGroup_Send(mySystem.axis[j].motor.evt_mot, EVT_MOT_REVERSE);
				}

				//绿灯闪
				led_set_event(j, (EVT_LEDx_FREQ_2HZ_50 | EVT_LEDx_BLINK_LOOP | EVT_LEDx_GREEN_ON));

				//回退完成位置环发送一键自动标定完成事件
			break;
		}
	}
}

//特殊机型获取
bool spc_model_get(uint8_t model)
{
	bool ret = false;
	switch(model)
	{
		case 16:
		case 17:
		case 35:
		case 36:
		case 161:
		case 162:
		case 167:
			if (true == mySystem.model_gk_1)
			{
				ret = true;
			}else
			{
				ret = false;
			}
			break;
		case 49:
			if (true == mySystem.model_ce_1)
			{
				ret = true;
			}else
			{
				ret = false;
			}
			break;
		default:
			break;
	}
	return ret;
}

//机型配置
void model_pos_upd(uint8_t idx)
{
	bool cur_logic_flag = false;

	//传感器异常就没必要保存位置了
	if (mySystem.axis[idx].pos_err == POS_NO)
	{
		return;
	}

	cur_logic_flag = spc_model_get(mySystem.model);

	work_config_modify(mySystem.model, cur_logic_flag);//存机型（内部会更新配置）
	work_config_init();//更新配置
	if (mySystem.axis[idx].dir == ACT_DIR_COMBINE_IS_MOVE_OUT)//防止更新过后还是标定失败
	{
		bsp_write_cali(idx, 1);
		bsp_write_pos_separate(idx, mySystem.axis[idx].pos_current);//存标定位置
		work_config_init();//更新配置
	}else if (mySystem.axis[idx].dir == ACT_DIR_COMBINE_IS_MOVE_IN)//防止更新过后还是标定失败
	{
		if (mySystem.axis[idx].pos_vol > 1250)
		{
			bsp_write_cali(idx, 1);
			bsp_write_pos_separate(idx, mySystem.axis[idx].pos_current);//存标定位置
			work_config_init();//更新配置
		}
	}

	//机型更新完毕更新下状态
	if (ACT_DIR_NONE == mySystem.axis[idx].dir)
	{
		EventGroup_Send(mySystem.axis[idx].evt_act, EVT_ACT_WORK_DISABLE);
		led_set_event(idx, EVT_LEDx_OFF);
		app_can_set_tx_mgr(idx, false);
	}else
	{
		EventGroup_Send(mySystem.axis[idx].evt_act, EVT_ACT_WORK_ENABLE);
		led_set_event(idx, EVT_LEDx_OFF);
		app_can_set_tx_mgr(idx, true);

		//执行完标定操作要把previous_state清掉，不然影响led显示
		mySystem.axis[idx].sm_act.previous_state = ACT_STATE_IDLE;

	}

}
