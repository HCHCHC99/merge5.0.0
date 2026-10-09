/**
 * @file    axis_typedef.h
 * @brief   控制器全局结构体、枚举、宏定义 最终整合版
 * @note    分层架构：系统层 -> 推杆轴层 -> 电机硬件层
 *          内嵌三环PID控制、状态机、事件组，业务与硬件低耦合
 *          无基础服务冗余耦合，全局定义统一收敛管理
 */
#ifndef __AXIS_TYPEDEF_H
#define __AXIS_TYPEDEF_H

#include <stdbool.h>
#include <stdint.h>
#include "state_engine.h"
#include "event_group.h"
#include "ring_buffer.h"
//#include "pid_common.h"
#include <stddef.h>

/**
 * @brief 根据结构体成员指针，获取整个结构体首地址
 * @param ptr    : 结构体某个成员的指针
 * @param type   : 完整结构体类型名
 * @param member : 结构体内部该成员的变量名
 * @return       : 结构体首地址指针
 */
#define container_of(ptr, type, member) \
    ( (type *)((char *)(ptr) - offsetof(type, member)) )

/**
 * @brief 结构体内部数组任意元素指针，自动求取结构体首地址
 * @param elem_ptr 数组中任意元素指针
 * @param type     结构体类型
 * @param member   结构体内部数组变量名
 */
//#define container_of_arr_any(elem_ptr, type, member)                         \
//    ((type *)((char *)(elem_ptr)                                            \
//        - offsetof(type, member)                                            \
//        - ((char *)(elem_ptr) - (char *)&(((type *)0)->member)) ))

#define container_of_arr_any(elem_ptr, type, member)                         \
    ((type *)((char *)(elem_ptr)                                           \
        - ((char *)(elem_ptr) - (char *)&(((type *)0)->member)) ))

/* ====================== 全局设备数量配置宏 ====================== */
#define MAX_AXIS_NUM               2
#define MAX_LED_NUM                2
#define MAX_KEY_NUM                2
#define MAX_COMM_NUM               1
#define MAX_STORAGE_NUM            1

/* ====================== 工具校验宏 ====================== */
#define VALID_AXIS_ID(id)          ((id) < MAX_AXIS_NUM)
#define VALID_LED_ID(id)           ((id) < MAX_LED_NUM)
#define VALID_KEY_ID(id)           ((id) < MAX_KEY_NUM)

/* ====================== 全局枚举定义 ====================== */
/**
 * @brief 电机硬件级故障码
 */
typedef enum {
    MOT_ERR_NONE = 0,
    MOT_ERR_OVER_CURRENT,
    MOT_ERR_OVER_TEMP,
    MOT_ERR_OVER_VOLTAGE,
} MotorError_t;

typedef enum {
    MOT_DIR_NONE = 0,
    MOT_DIR_FORWARD,
    MOT_DIR_REVERSE,
    MOT_DIR_MAX,
} Motor_Dir_t;

typedef enum {
    MOT_DISABLE = 0,
    MOT_ENABLE
} Motor_Enable_t;

typedef enum {
    ARM_INDEX_HU = 0,
    ARM_INDEX_HV,
    ARM_INDEX_MAX
} Arm_Index_t;

typedef enum {
    MOT_INDEX_0 = 0,
    MOT_INDEX_1,
    MOT_INDEX_MAX
} Mot_Index_t;

// ==============================
// 上桥臂 硬件配置
// ==============================
typedef struct {
    uint8_t  gpio_port;
    uint16_t pin;
    uint8_t active_level;
} Mot_HwConfig_t;

// 一个电机 = 一组 HU + HV
typedef struct {
    const Mot_HwConfig_t *hu;  // 上桥臂U
    const Mot_HwConfig_t *hv;  // 上桥臂V
} Motor_Hw_t;

/**
 * @brief 推杆轴业务级故障码
 */
typedef enum {
    AXIS_ERR_NONE = 0,
    AXIS_ERR_OVER_CURRENT,
    AXIS_ERR_OVER_TEMP,
    AXIS_ERR_OUT_OF_RANGE,
    AXIS_ERR_STALL,
} AxisError_t;

/**
 * @brief 系统整机运行模式
 */
typedef enum {
    SYS_MODE_IDLE,
    SYS_MODE_NORMAL,           // 正常模式
    SYS_MODE_CALIB_MODE_1,     // 手动标定1
    SYS_MODE_CALIB_MODE_2,     // 手动标定2
    SYS_MODE_AUTO_CALIB,       // can自动标定
    SYS_MODE_MAX
} SystemMode_t;

/* -------------------------- 类型枚举 -------------------------- */
typedef enum {
    PID_ALG_POSITION = 0,
    PID_ALG_INCREMENT,
} PID_AlgType_t;

typedef enum {
    FAULT_NONE = 0,
    FAULT_OVER_CURRENT,
    FAULT_OVER_TEMP,
    FAULT_OUT_OF_RANGE,
    FAULT_STALL,
} FaultType_t;

/* -------------------------- PID 结构体 -------------------------- */
typedef struct {
    PID_AlgType_t alg_type;
    float kp;
    float ki;
    float kd;

    float integral_max;   // 积分限幅
} PID_Param_t;

typedef struct {
    bool  enable;         // 使能
    float set;            // 设定值
    float fdb;            // 反馈值
    uint32_t fdb_time;    // 反馈值 时间戳

    float fdb_last;       // 上一次 反馈值
    uint32_t fdb_last_time;    // 上一次 反馈值 时间戳

    float out;            // 输出值
    float out_nor;        // 输出值 方向不同归一化处理系数 ±1.0

    float out_max;        // 输出上限
    float out_min;        // 输出下限

    float err;            // 当前误差
    float err_last;       // 上一次误差
    float err_last_2;     // 上上一次误差
    float integral;       // 积分值

    float stall_thr;    // 偏差 阈值

    float over_current_thr;    // 堵转过流判断阈值
    float limit_current_thr;   // 260718_add:限位断电判断阈值

    bool forward_is_forced;
    bool reverse_is_forced;

} PID_Ctrl_t;

/* ====================== 硬件层：电机结构体 ====================== */
/**
 * @brief 电机硬件实体
 * @note 仅承载硬件采样、PWM、硬件故障、电机级状态机与事件组
 *       纯硬件抽象，不掺杂业务逻辑
 */
typedef struct {
    uint8_t id;
    bool enable;
    // bool  is_enabled;
    bool  is_breaked;
    // uint8_t enable;

    /* 硬件物理量 */
    float current;
    float voltage;
    float temp;

    int16_t rpm;
    uint8_t pwm_duty;

    /* 硬件故障 */
    MotorError_t error;

    Motor_Dir_t  dir;
    Motor_Dir_t  dir_last;

    const Motor_Hw_t *hw;

    /* 状态机与事件组 */
    StateMachine_t sm_mot;
    EventGroup_t *evt_mot;
} Motor_t;

/**
 * @brief 推杆运行模式
 */
typedef enum {
    ACT_MODE_IDLE,
    ACT_MODE_NORMAL,
    ACT_MODE_CALIB_MODE,
    ACT_MODE_MAX
} AxisMode_t;

/**
 * @brief 推杆 动作方向 定义
 */
typedef struct {
    float speed_value;            // 推杆速度
    float pos;                    // 位置1
    // float pos_2;                  // 位置2

    float   speed_limit;          // 速度上限
    uint32_t timeout;             // 时间戳 记录
    uint32_t timeout_thr;         // 超时阈值，计算速度
} AxisSpeed_t;

/**
 * @brief 推杆 动作方向 定义
 */
typedef enum {
    ACT_DIR_NONE,
    ACT_DIR_COMBINE_IS_MOVE_OUT,
    ACT_DIR_COMBINE_IS_MOVE_IN,
    ACT_DIR_MAX
} AxisDir_t;

/**
 * @brief 推杆 位置 定义
 */
typedef enum {
    ACT_POS_MIDDLE,
    ACT_POS_COMBINE,
    ACT_POS_SEPARATE,
    ACT_POS_MAX
} AxisPos_t;

typedef enum {
	POT_NORMAL = 0,
	POS_ERR = 1,	//位置错误
	POS_NO = 2,		//掉线
} pot_stat_t;
/* ====================== 业务控制层：推杆轴结构体 ====================== */
/**
 * @brief 推杆轴业务实体
 * @note 承载行程、限位、位置速度、业务故障
 *       挂载电机实体 + 内嵌位置/速度/电流三环PID
 *       自带轴级状态机与事件组
 */
typedef struct {
    uint8_t id;
    uint8_t enable;

    bool swt_enable;
    bool swt_start;
    uint32_t swt_count;
    uint32_t stall_count;
    uint32_t stall_count_thr;
    uint32_t success_count;
    uint32_t success_count_thr;

    /* 260625_RL_add:新增下面两个字段*/
    uint32_t startup_mask_count;   // 启动屏蔽计数器（每200μs +1）
    uint32_t startup_mask_thr;     // 启动屏蔽阈值 N*200us N=2500（即 500ms）

    float pos_combine;             // 位置数据：结合目标位置
    float pos_separate;            // 位置数据：分离目标位置
    float pos_current;             // 位置数据：当前位置
    // float pos_target;              // 位置数据：目标位置

    AxisMode_t mode;

    AxisDir_t  dir;                // 方向逻辑：ACT_DIR_COMBINE_IS_MOVE_OUT == 结合方向为 推杆伸出方向
    AxisPos_t  position;           // 位置信息：结合点、分离点、中间位置

    AxisSpeed_t speed;             // 推杆速度
    RingBufferObj *speed_data;     // 速度采样环形缓冲句柄

    /* 轴业务故障 */
    AxisError_t error;
    /* 挂载底层电机硬件实体 */
    Motor_t motor;

    /* ====================== 运动控制三环PID 整合内嵌 ====================== */
    PID_Param_t   pos_param;
    PID_Ctrl_t    pos_ctrl;

    PID_Param_t   spd_param;
    PID_Ctrl_t    spd_ctrl;

    PID_Param_t   curr_param;
    PID_Ctrl_t    curr_ctrl;

    /* 状态机与事件组 */
    StateMachine_t sm_act;
    EventGroup_t *evt_act;

    /* can状态机与事件组 */
    StateMachine_t sm_can;
    EventGroup_t *evt_can;

    /* 标定状态机与事件组 */
    StateMachine_t sm_calib;
    EventGroup_t *evt_calib;

	//标定状态标志
	bool	is_calib;

	//标定状态标志
	bool	is_calib_last;

	//传感器故障标志
	pot_stat_t	pos_err;
	//传感器故障标志
	pot_stat_t	pos_err_last;
	// 复制一份 acc状态，便于操作
    bool     acc_enable;
	//电流
	uint16_t current;

    //260718_add:上、下电机相线断电点对应的位置
    float  pos_top_limit;
    float  pos_down_limit;
    uint32_t limit_count_thr;

	bool   act_stall_flag;

	uint16_t pos_vol;
} Axis_t;

/* ====================== 顶层系统结构体 ====================== */
/**
 * @brief 系统整机顶层实体
 * @note 全局统筹管理，不下沉单轴细节
 *       系统级状态机、事件组、整机参数、多轴数组管理
 */
typedef struct {
    /* 系统状态与模式 */
    StateMachine_t sys_sm;
    EventGroup_t *sys_evt;
    SystemMode_t mode;

    /* 整机全局参数 */
    float voltage;
    float temp;
    uint8_t global_enable;
    uint32_t error_code;

	//钥匙电
	bool acc_enable;
	bool key_over_gear_enable;
	bool can_over_gear_enable;
	bool over_gear_enable_last;
	bool over_spd_enable;
	bool over_spd_enable_last;

	//手动标定状态标志
	bool	is_manu_calib;
	bool	manu_calib_motor_idx;//正在标定的

	//机型
	uint8_t model;//指程序中实时获取到的，在一定条件下会被保存并更新到有效机型
	//机型
	uint8_t eff_model;//指系统中被存储的，符合当前系统实际逻辑的

	//第一次标定状态标志 -- 只管当前上电的，不需要存储
	//特殊机型标志
	bool  model_gk_1;
	//特殊机型标志
	bool  model_ce_1;
	bool  first_calib;

	uint8_t target_id ;
    /* 多轴实例数组 */
    Axis_t axis[MAX_AXIS_NUM];

} System_t;

/* 全局系统根实体声明 */
extern System_t mySystem;
extern const Motor_Hw_t MOTOR_HW_LIST[MOT_INDEX_MAX];
#endif
